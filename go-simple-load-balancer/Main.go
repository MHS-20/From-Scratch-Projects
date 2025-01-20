package main

import (
	"fmt"
	"net/http"
	"net/http/httputil"
	"net/url"
	"sync"
	"sync/atomic"
)

type Backend struct {
	URL          *url.URL
	Alive        bool
	mux          sync.RWMutex
	ReverseProxy *httputil.ReverseProxy
}

type ServerPool struct {
	backends       []*Backend
	current_server uint64
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

// atomic round robin
func (s *ServerPool) NextIndex() int {
	return int(atomic.AddUint64(&s.current_server, uint64(1)) % uint64(len(s.backends)))
}

func (s *ServerPool) GetNextPeer() *Backend {

	idx := 0

	// loops until a server is available
	for {
		idx = s.NextIndex()
		if s.backends[idx].isAlive() {
			atomic.StoreUint64(&s.current_server, uint64(idx)) // mark as current server
			return s.backends[idx]
		}
	}
}

func LoadBalancing(w http.ResponseWriter, r *http.Request) {
	peer := serverPool.GetNextPeer()
	peer.ReverseProxy.ServeHTTP(w, r)
	http.Error(w, "Service not available", http.StatusServiceUnavailable)
}

var serverPool ServerPool
var port int

func main() {
	// initialize reverse proxy
	u, _ := url.Parse("http://localhost:8080")
	proxy := httputil.NewSingleHostReverseProxy(u)
	http.HandlerFunc(proxy.ServeHTTP)

	server := http.Server{
		Addr:    fmt.Sprintf(":%d", port),
		Handler: http.HandlerFunc(LoadBalancing),
	}

}
