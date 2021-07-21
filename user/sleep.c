// Sleep
// 在用户指定的时钟数里，这个程序会暂停。
// tick 是由 xv6 kernel 内核定义的时间概念。也可以说是时间片的两次中断之间的时间。
// 这里并没有给 sleep 做定义

#include "kernel/types.h"   // types.h 里面有 user.h 要包含的 define
#include "user/user.h"


int 
main(int argc, char* argv[])
{
    if(argc < 2) {
        fprintf(2, "Error: no argument...\n");
        exit(1);    // exit the current process in the zombie state until its parent calls wait()
    }
    else if(argc > 2) {
        fprintf(2, "Error: too many arguments...\n");
        exit(1);
    }
    else{
        int ticks = atoi(argv[1]);
        sleep(ticks);
        exit(0);
    }
    exit(0);
}