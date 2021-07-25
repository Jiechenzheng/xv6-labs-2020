// sleep
// parent -> send byte
// child -> receive byte; print
// child -> send byte
// parent -> receive byte, print

#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char* argv[]){
    char* msgparent = "ping";
    char* msgchild = "pong";
    int msgsize = 5;    

    int p1[2];  // two file descriptors from pipe
    int p2[2];  // 用两个 pipes 来做互相交流
    int pid;

    if (pipe(p1) < 0) exit(1);
    if (pipe(p2) < 0) exit(1);

    // fork a child process
    pid = fork();   // 这个 pid 只是 fork 的返回值，并不是这个进程真正的 pid

    // 要考虑到父与子进程的同步问题， 但是我用了两个 pipes 之后就不用了
    if (pid > 0) {
        // write to the pipe
        write(p1[1], msgparent, msgsize);
        
        // read to the pipe
        char inbuf[msgsize];
        read(p2[0], inbuf, msgsize);
        printf("%d: received pong\n", getpid());
        exit(0);
    }
    else if (pid == 0) {
        // read and print
        char inbuf[msgsize];
        read(p1[0], inbuf, msgsize);
        printf("%d: received ping\n", getpid());
        
        // write a message
        write(p2[1], msgchild, msgsize);
        exit(0);
    }

    return 0;
}