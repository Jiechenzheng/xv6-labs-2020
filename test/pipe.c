// C program to illustrate pipe system call in C
#include <stdio.h>
#include <unistd.h> // 这个库包含了 POSIX 系统 API 访问功能的方法。C 和 C++ 都可以
#include <stdlib.h>

#define MSGSIZE 16

char* msg1 = "hello, world #1";
char* msg2 = "hello, world #2";
char* msg3 = "hello, world #3";

int main()
{
    char inbuf[MSGSIZE];
    int p[2], i;

    if (pipe(p) < 0) exit(1);   // 这边执行了 pipe(p) 这个方法

    /* write pipe */
    write(p[1], msg1, MSGSIZE);
    write(p[1], msg2, MSGSIZE);
    write(p[1], msg3, MSGSIZE);
    printf("p[1] equals to %d \n", p[1]);

    for (int i = 0; i < 3; i++)
    {
        read(p[0], inbuf, MSGSIZE);
        printf("%s\n", inbuf);
    }

    return 0;
    
}