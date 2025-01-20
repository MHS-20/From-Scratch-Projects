

## Simple Container in Go

Simple implementation of a container.
It creates isolation at these levels: hostname, PID namespace and filesystem isolation.
A copy of the root filesystem is needed by the container, at `` /home/newroot ``.

Usage: 
```    
go run main.go run cmd args 
```