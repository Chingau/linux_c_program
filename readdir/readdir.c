#include <stdio.h>
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>

#define MY_DIRENT "/home/gaoxu/workspace/gitlib/linux_c_program/ipc/posix_mq"

int list_file(const char *base_dir)
{
    DIR *dirp;
    struct dirent* dp;
    int count = 0;

    // 打开目录
    if ((dirp = opendir(base_dir)) == NULL) {
        perror("opendir error");
        return -1;
    }
    
    printf("sizeof(dirent) = %ld\n", sizeof(struct dirent) - 256);
    printf("%-20s %10s %25s %15s %15s %-s\n", "type", "d_ino", "d_off", "d_reclen", "len", "filename");

    while ((dp = readdir(dirp)) != NULL) {
        count++;
        // 忽略当前目录"."和上一级目录".."（父目录）
        if (0 == strcmp(dp->d_name, ".") || 0 == strcmp(dp->d_name, ".."))
            continue;

        // 读取文件类型
        char type[50];
        switch (dp->d_type) {
            case DT_DIR: // a directory
                snprintf(type, sizeof(type), "%s", "directory");
                break;
            case DT_REG: // a regular file
                snprintf(type, sizeof(type), "%s", "regular file");
                break;
            case DT_BLK: // a block device
                snprintf(type, sizeof(type), "%s", "block device");
                break;
            case DT_CHR: // a character device
                snprintf(type, sizeof(type), "%s", "character device");
                break;
            case DT_FIFO: // a named pipe (FIFO)
                snprintf(type, sizeof(type), "%s", "named pipe (FIFO)");
                break;
            case DT_LNK: // a symbolic link
                snprintf(type, sizeof(type), "%s", "symbolic link");
                break;
            case DT_SOCK: // a UNIX domain socket
                snprintf(type, sizeof(type), "%s", "UNIX domain socket");
                break;
            default: // DT_UNKNOWN - file type unknown
                snprintf(type, sizeof(type), "%s", "file type unknown");
                break;
        }
        printf("%-20s %10lu %25ld %15u %15ld %-s\n", type, dp->d_ino, dp->d_off, dp->d_reclen, strlen(dp->d_name), dp->d_name);
    }

    printf("count = %d\n", count);
    // 关闭目录
    closedir(dirp);
    return 0;
}


int main(int argc, char *argv[])
{
    list_file(MY_DIRENT);
    return 0;
}
