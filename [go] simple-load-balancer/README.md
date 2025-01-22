## Simple Load Balancer in Go
Simple implementation of a load balancer in Go.It uses a Round-Robin as balancing algorithm.
It supports health checks for the backend servers and it uses a context to count retries and attempts for each request.

Usage: 
```    
go run main.go port url1 url2 ...
```

Example: 
```
go run main.go 8080 http://localhost:8081 http://localhost:8082
```