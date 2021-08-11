// xargs
// read lines from the standard input and run a command for each line.

#include "kernel/types.h"
#include "user/user.h"
#include "kernel/param.h"

#define MAXSIZE 32

int main(int argc, char *argv[])
{
    // 先把命令行里面 xargs 后面的 command 读入到数组里
    char *command[MAXARG];
    int commandguard = argc - 2;
    for (int i = 1; i < argc; i++){ // argv 的第一个是 name of program, 即 `xargs`
        command[i-1] = argv[i]; // ？ 如何保证 command 里面每一个元素的 size 呢？
    }

    // 获取 standard input 里面的字符串， 遇到 '\n' 就暂停读取并 exec
    char buf;
    char token[MAXSIZE];
    int tokenguard = -1; // 哨兵，表示 token 中第几个位置有值
    while (read(0, &buf, 1))    // 输入流， 也就是前一个 command 的结果
    {
        if (buf == ' ' || buf == '\n'){
            if (++commandguard < MAXARG){
                token[++tokenguard] = 0;
                command[commandguard] = token;
                tokenguard = -1;
                
                if (buf == '\n'){
                    command[++commandguard] = 0;
                    commandguard = argc - 2;
                    if (fork() == 0){
                        exec(command[0], command);
                    }
                    else {
                        wait(0);
                    }
                    command[commandguard+1] = 0;
                }
            } 
            else{
                printf("Too many xargs command arguments..");
                exit(1);
            }
        }
        else {
            if (++tokenguard < MAXSIZE){
                token[tokenguard] = buf;
            }
            else {
                printf("The argument is too long: %s..", token);
            }
        }
    }
    
    if (fork() == 0) {
        exec(command[0], command);
    }
    else {
        wait(0);
    }
    exit(0);
}
