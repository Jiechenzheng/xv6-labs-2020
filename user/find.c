// find
// 做一个 find 的功能,可查找文件也可查找文件夹

#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/stat.h"


char* 
fmtname(char *path)
{
//   static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  return p;
}

void 
find(char *path, char filename[]) 
{
    char buf[512], *p;
    int fd; // file descriptor
    struct dirent de;   // directory 里面包含的它包括的文件信息
    struct stat st;     // 文件的状态 state

    // printf("The path is: %s\n", path);

    if ((fd = open(path, 0)) < 0){  // 如果能打开的话，这一步就已经把它打开了
        fprintf(2, "ls: cannot open %s\n", path);
        exit(1);
    }

    if (fstat(fd, &st) < 0) {
        fprintf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }

    // 与 filename 比较
    // printf("the path is: %s\n", path);
    // printf("fmtname is: %s;filename is: %s\n", fmtname(path), filename);
    if (strcmp(fmtname(path), filename) == 0) {
        printf("%s\n", path);
    }

    switch (st.type)
    {
    // 如果 path 是文件，直接返回
    case T_FILE:
        break;
    // 如果 path 是 directory, 找到它下面的子树，继续 find();
    case T_DIR:
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
            printf("ls: path is too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf+strlen(buf);
        *p++ = '/';
        read(fd, &de, sizeof(de));  // 读取第一个包含它自己的信息
        int selfinum = de.inum;
        while (read(fd, &de, sizeof(de)) == sizeof(de)){
            if(de.inum <= selfinum) continue;    // 意思是这个 dir 里面是没有文件夹以及文件的
            memmove(p, de.name, DIRSIZ);    // 把这个文件夹加上 / 再加其下面的文件
            p[DIRSIZ] = 0;
            if(stat(buf, &st) < 0){
                printf("ls: cannot stat %s\n", buf);
                continue;
            }
            // TODO, 有一个 bug 应该是搜索完最后一个文件后程序没有正常退出
            find(buf, filename);
        }
        break;
    }
    close(fd);
    return;
}

int 
main (int argc, char* argv[])
{
    if (argc < 2) {      // 命令行里只有 'find'
        printf("Not enough arguments..");
        return 0;
    }
    else if (argc == 2) {
        printf("Not enough arguments...");
        return 0;
    }
    else if (argc == 3) {
        // 着重补充这个吧，上面两个可以直接把 ls 的方法都复制过来了
        find(argv[1], argv[2]);
        return 0;
    }

    return 0;
}