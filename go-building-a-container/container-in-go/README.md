

## Simple Container in Go

Simple implementation of a container.


It creates some level of isolation: hostname, PID namespace and filesystem isolation.

A copy of the root FS is needed by the container, at `` /home/newroot ``.

Usage: 
```    
go run main.go run cmd args 
```