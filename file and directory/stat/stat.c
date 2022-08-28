#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

int main(int argc, const char *argv[])
{
    if (argc < 2) {
        printf("./a.out filename\n");
        exit(1);
    }

    struct stat st;
    if ((stat(argv[1], &st)) < 0) {
        perror("stat");
        exit(1);
    }

    char perms[11] = {0}; //存储文件类型和访问权限

    //判断文件类型
    switch(st.st_mode & S_IFMT) {
        case S_IFLNK:
        perms[0] = 'l';
        break;

        case S_IFDIR:
        perms[0] = 'd';
        break;

        case S_IFREG:
        perms[0] = '-';
        break;

        case S_IFBLK:
        perms[0] = 'b';
        break;

        case S_IFCHR:
        perms[0] = 'c';
        break;

        case S_IFSOCK:
        perms[0] = 's';
        break;

        case S_IFIFO:
        perms[0] = 'p';
        break;

        default:
        perms[0] = '?';
        break;
    }

    //判断文件权限
    perms[1] = (st.st_mode & S_IRUSR) ? 'r' : '-';
    perms[2] = (st.st_mode & S_IWUSR) ? 'w' : '-';
    perms[3] = (st.st_mode & S_IXUSR) ? 'x' : '-';

    perms[4] = (st.st_mode & S_IRGRP) ? 'r' : '-';
    perms[5] = (st.st_mode & S_IWGRP) ? 'w' : '-';
    perms[6] = (st.st_mode & S_IXGRP) ? 'x' : '-';

    perms[7] = (st.st_mode & S_IROTH) ? 'r' : '-';
    perms[8] = (st.st_mode & S_IWOTH) ? 'w' : '-';
    perms[9] = (st.st_mode & S_IXOTH) ? 'x' : '-';

    int link_num = st.st_nlink; //硬链接计数
    char *file_user = getpwuid(st.st_uid)->pw_name; //文件所有者
    char *file_grp = getgrgid(st.st_gid)->gr_name; //文件所属组
    int file_size = (int)st.st_size; //文件大小
    char *time = ctime(&st.st_mtime); //修改时间
    char mtime[512] = {0};
    strncpy(mtime, time, strlen(time)-1);

    char buf[1024];
    sprintf(buf, "%s %d %s %s %d %s %s", perms, link_num, file_user, file_grp, file_size, mtime, argv[1]);
    printf("%s\n", buf);

    return 0;
}