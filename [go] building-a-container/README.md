## Simple Container in Go
Simple implementation of a container with cgroups and namespaces.
It isolates the hostname, the PID namespace and the filesystem.
A copy of the root filesystem is needed by the container, at `` /home/newroot ``.

Usage: 
```    
go run main.go run cmd args 
```