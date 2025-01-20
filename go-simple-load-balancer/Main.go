package main

import (
	"context"
	"fmt"
	"log"
	"net"
	"net/http"
	"net/http/httputil"
	"net/url"
	"os"
	"strconv"
	"sync"
	"sync/atomic"
	"time"
)

// ----------------------
// ----- LB Backend -----
// ----------------------

type Backend struct {
	URL          *url.URL
	Alive        bool
	mux          sync.RWMutex
	ReverseProxy *httputil.ReverseProxy
}

func (b *Backend) SetAlive(alive bool) {
	b.mux.Lock()
	b.Alive = alive
	b.mux.Unlock()
}

func (b *Backend) isAlive() (alive bool) {
	b.mux.RLock()
	alive = b.Alive
	b.mux.RUnlock()
	return
}

// -----------------------
// ----- Server Pool -----
// -----------------------
type ServerPool struct {
	backends       []*Backend
	current_server uint64
}

func (s *ServerPool) AddBackend(backend *Backend) {
	s.backends = append(s.backends, backend)
}

// atomic round robin
func (s *ServerPool) NextIndex() int {
	return int(atomic.AddUint64(&s.current_server, uint64(1)) % uint64(len(s.backends)))
}

// loops until a server is available
func (s *ServerPool) GetNextPeer() *Backend {
	var idx int
	for {
		idx = s.NextIndex()
		if s.backends[idx].isAlive() {
			atomic.StoreUint64(&s.current_server, uint64(idx)) // mark as current server
			return s.backends[idx]
		}
	}
}

// changes a status of a backend
func (s *ServerPool) MarkBackendStatus(backendUrl *url.URL, alive bool) {
	for _, b := range s.backends {
		if b.URL.String() == backendUrl.String() {
			b.SetAlive(alive)
			break
		}
	}
}

func (s *ServerPool) areHealthy() {
	var status string
	for _, b := range s.backends {
		alive := isBackendAlive(b.URL)
		b.SetAlive(alive)

		// log
		if !alive {
			status = "down"
		} else {
			status = "up"
		}

		log.Printf("%s [%s]\n", b.URL, status)
	}
}

// ----------------------
// ----- CONTEXT --------
// ----------------------
// used to count retries and attempts

const (
	Attempts int = iota
	Retry
)

// return the retries for a request
func GetRetryFromContext(r *http.Request) int {
	retry, ok := r.Context().Value(Retry).(int)
	if ok {
		return retry
	}

	return 0
}

// returns the attempts for a request
func GetAttemptsFromContext(r *http.Request) int {
	attempts, ok := r.Context().Value(Attempts).(int)
	if ok {
		return attempts
	}
	return 1
}

// ------------------------
// ----- HEALTH CHECKS ----
// ------------------------
func isBackendAlive(u *url.URL) bool {
	timeout := 2 * time.Second
	conn, err := net.DialTimeout("tcp", u.Host, timeout)

	if err != nil {
		log.Println("Server unreachable, error: ", err)
		return false
	}

	conn.Close()
	return true
}

func healthCheck() {
	t := time.NewTicker(time.Second * 20)
	for {
		select {
		case <-t.C:
			log.Println("Starting health check...")
			serverPool.areHealthy()
			log.Println("Health check completed")
		}
	}
}

// ---------------------------
// ----------- MAIN ----------
// ---------------------------
var serverPool ServerPool

func LoadBalancing(w http.ResponseWriter, r *http.Request) {
	attempts := GetAttemptsFromContext(r)
	if attempts > 3 {
		log.Printf("%s(%s) Max attempts reached, terminating\n", r.RemoteAddr, r.URL.Path)
		http.Error(w, "Service not available", http.StatusServiceUnavailable)
		return
	}

	peer := serverPool.GetNextPeer()
	peer.ReverseProxy.ServeHTTP(w, r)
	http.Error(w, "Service not available", http.StatusServiceUnavailable)
}

func myErrorHandler(proxy *httputil.ReverseProxy, serverUrl *url.URL, writer http.ResponseWriter, request *http.Request, e error) {
	log.Printf("[%s] %s\n", serverUrl.Host, e.Error())
	retries := GetRetryFromContext(request)
	if retries < 3 {

		// delay channel
		select {
		case <-time.After(10 * time.Millisecond):
			ctx := context.WithValue(request.Context(), Retry, retries+1)
			proxy.ServeHTTP(writer, request.WithContext(ctx))
		}
		return
	}

	// after 3 retries, mark this backend as down
	serverPool.MarkBackendStatus(serverUrl, false)

	// try a different backends, increase attempt count
	attempts := GetAttemptsFromContext(request)
	log.Printf("%s(%s) Attempting retry %d\n", request.RemoteAddr, request.URL.Path, attempts)
	ctx := context.WithValue(request.Context(), Attempts, attempts+1)
	LoadBalancing(writer, request.WithContext(ctx))
}

func main() {

	// Parse args
	args := os.Args[1:]
	if len(args) < 1 {
		log.Fatal("Please provide a port and at least one backend URL")
	}

	// Parse the port
	port, err := strconv.Atoi(args[0])
	if err != nil {
		log.Fatal("Invalid port value")
	}
	log.Printf("Server will run on port: %d", port)

	// The remaining arguments are URLs (backends)
	if len(args) < 2 {
		log.Fatal("Please provide at least one backend URL")
	}

	// Parse backend URLs, initialize proxies and server pool
	serverList := args[1:]
	for _, tok := range serverList {
		serverUrl, err := url.Parse(tok)

		if err != nil {
			log.Fatal(err)
		} else {
			log.Println("Parsed server URL:", serverUrl)
		}

		// initialize proxy and error handler
		proxy := httputil.NewSingleHostReverseProxy(serverUrl)
		proxy.ErrorHandler = func(writer http.ResponseWriter, request *http.Request, e error) {
			myErrorHandler(proxy, serverUrl, writer, request, e)
		}

		// initialize server pool
		serverPool.AddBackend(&Backend{
			URL:          serverUrl,
			Alive:        true,
			ReverseProxy: proxy,
		})
		log.Printf("Configured server: %s\n", serverUrl)
	}

	// LB http server
	server := http.Server{
		Addr:    fmt.Sprintf(":%d", port),
		Handler: http.HandlerFunc(LoadBalancing),
	}

	// start health checking
	go healthCheck()

	log.Printf("Load Balancer started at :%d\n", port)
	if err := server.ListenAndServe(); err != nil {
		log.Fatal(err)
	}

}
