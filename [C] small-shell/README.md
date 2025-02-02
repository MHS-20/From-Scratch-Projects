
## Small Shell
Simple implementation of a shell in C. It reads input from the user, tokenizes and executes the command using the execve system call. The command can be both a relative or absolute path to an executable file.

The shell assumes that the PATH environment variable is set and contains valid directories.

It supports the following built-in commands:
- exit
- cd
- help
- env


<br>
Usage: 

```
gcc *.c -o shell && ./shell
```