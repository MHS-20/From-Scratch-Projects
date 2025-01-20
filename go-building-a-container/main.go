package main

import (
	"fmt"
	"io/ioutil"
	"os"
	"os/exec"
	"path/filepath"
	"strconv"
	"syscall"
)

// CLI:
// Docker Engine:   docker run <container> cmd args
// This file:       go run main.go run cmd args

// We need a copy of the root FS, it will be used by the container:
// sudo mkdir -p /home/newroot
// sudo rsync -a --exclude=/home / /home/newroot

func main() {
	switch os.Args[1] {
	case "run":
		run()
	case "child":
		child()
	default:
		panic("oh")
	}
}

func run() {
	// basically a fork-exec, call the child func
	cmd := exec.Command("/proc/self/exe", append([]string{"child"}, os.Args[2:]...)...)

	cmd.Stdin = os.Stdin
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr

	// create new namespaces for the container
	cmd.SysProcAttr = &syscall.SysProcAttr{
		Cloneflags:   syscall.CLONE_NEWUTS | syscall.CLONE_NEWPID | syscall.CLONE_NEWNS | syscall.CLONE_NEWUSER,
		Unshareflags: syscall.CLONE_NEWNS, // unshare the FS and user namespaces from the parent

		// map to root user inside the container
		Credential: &syscall.Credential{Uid: 0, Gid: 0},
		UidMappings: []syscall.SysProcIDMap{
			{ContainerID: 0, HostID: os.Getuid(), Size: 1},
		},
		GidMappings: []syscall.SysProcIDMap{
			{ContainerID: 0, HostID: os.Getgid(), Size: 1},
		},
	}

	must(cmd.Run())
}

func child() {
	fmt.Printf("running %v as PID %d\n", os.Args[2:], os.Getpid())

	cg() // cgroups

	cmd := exec.Command(os.Args[2], os.Args[3:]...)
	cmd.Stdin = os.Stdin
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr

	// create chroot jail (limited view of the FS)
	must(syscall.Sethostname([]byte("container")))
	must(syscall.Chroot("/home/newroot"))
	must(syscall.Chdir("/"))

	// isolated PID list
	must(syscall.Mount("proc", "proc", "proc", 0, ""))

	must(cmd.Run())
}

func cg() {
	cgroups := "/sys/fs/cgroup/"
	pids := filepath.Join(cgroups, "pids")

	must(os.Mkdir(filepath.Join(pids, "dude"), 0755))
	must(ioutil.WriteFile(filepath.Join(pids, "dude/pids.max"), []byte("20"), 0700)) // max 20 processes

	// Removes the new cgroup in place after the container exits
	must(ioutil.WriteFile(filepath.Join(pids, "dude/notify_on_release"), []byte("1"), 0700))
	must(ioutil.WriteFile(filepath.Join(pids, "dude/cgroup.procs"), []byte(strconv.Itoa(os.Getpid())), 0700)) // add the current process to the cgroup
}

func must(err error) {
	if err != nil {
		panic(err)
	}
}
