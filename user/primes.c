// primes
// 用 pipeline 做一个素数筛子去筛选素数

#include "kernel/types.h"
#include "user/user.h"

static void primeseive(int readfile);

int main(int argc, char* argv[]) {
    int number;
    int p[2];

    if (pipe(p) < 0) exit(1);
    
    for (number = 2; number < 35; number++)
    {
        write(p[1], &number, 4);
    }

    close(p[1]);
    // 还是要想一想如何递归转循环
    primeseive(p[0]);
    
    return 0;
}

static void primeseive(int readfile) {
    int pivot;
    
    if (!read(readfile, &pivot, 4)){
        close(readfile);
        return;
    } 

    int pid = fork();

    if (pid > 0) {
        wait((int*) 0);
        exit(0);
        return;
    }
    else if (pid == 0){
        printf("prime %d\n", pivot);
        int inbuf;
        int p[2];
        if (pipe(p) < 0) exit(1);
        
        while (read(readfile, &inbuf, 4))    // 把文件的数据读到 inbuf 里
        {
            if (inbuf % pivot != 0){
                write(p[1], &inbuf, 4);      // 把 inbuf 的数据写到文件上
            }
        }

        close(readfile);
        close(p[1]);
        primeseive(p[0]);
        exit(0);
    }
    else {
        printf("fork error\n");
    }
}