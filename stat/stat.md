# stat 函数(stat, fstat, lstat)

`stat`函数用于获取文件信息； 

所需要的头文件：

```c
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
```

函数原型：

```c
int stat(const char *pathname, struct stat *buf);
//成功返回0，失败返回-1,并更新errno
//pathname:指出文件(文件路径)
//buf:输出参数(函数对该参数操作，然后传出)
```

```c
int fstat(int fd, struct stat *buf);
//fstat 函数与 stat 函数功能一样，只是第一个参数是文件描述符
```

```c
int lstat(const char *pathname, struct stat *buf);
```

`lstat` 函数的形参跟 stat 函数的形参一样。其功能也跟 stat 函数功能一样，仅有一点不同：stat 函数是穿透(追踪)函数，即对软链接文件进行操作时，操作的是链接的那一个文件，不是软链接文件本身；而 lstat 函数是不穿透(追踪)函数，对软链接文件进行操作时，操作的是软链接的文件本身。

以上三个函数：成功返回0，失败返回-1，并将错误信息更新到 errno 变量中。

> 其它 Linux 系统函数类似，带 `l` 表示不追踪，不带 `l` 表示追踪(穿透)。如：`ls -l` 命令查看文件属性是不追踪的；`rm` 删除文件时，是追踪的；对于穿透的命令，是无法判断文件是不是软链接文件，比如 `ls -l` 命令，其是不穿透的，因此可以判断是否是软链接文件；如果是用 `stat` 函数实现 `ls -l` 命令，则是穿透的，对于查看原文件的链接文件的属性是一样的，无法区别两者，因此可以考虑用 `lstat` 函数来实现 `ls -l` 命令的功能。

`struct stat` 结构体如下：

```c
struct stat
{
    dev_t     st_dev;     /* ID of device containing file */文件使用的设备号
    ino_t     st_ino;     /* inode number */    索引节点号 
    mode_t    st_mode;    /* protection */  文件对应的模式，文件，目录等
    nlink_t   st_nlink;   /* number of hard links */    文件的硬连接数  
    uid_t     st_uid;     /* user ID of owner */    所有者用户识别号
    gid_t     st_gid;     /* group ID of owner */   组识别号  
    dev_t     st_rdev;    /* device ID (if special file) */ 设备文件的设备号
    off_t     st_size;    /* total size, in bytes */ 以字节为单位的文件容量   
    blksize_t st_blksize; /* blocksize for file system I/O */ 包含该文件的磁盘块的大小   
    blkcnt_t  st_blocks;  /* number of 512B blocks allocated */ 该文件所占的磁盘块  
    time_t    st_atime;   /* time of last access */ 最后一次访问该文件的时间   
    time_t    st_mtime;   /* time of last modification */ /最后一次修改该文件的时间   
    time_t    st_ctime;   /* time of last status change */ 最后一次改变该文件状态的时间   
};
```

> struct stat 结构体位于 inode(索引节点)中，但是其内部不包含文件名。文件名位于文件的目录项 dentry 中，其包含文件名的 inode 编号。通过 dentry 和 inode 编号可以找到 inode，进一步找到文件本身。硬链接就是 dentry(目录项)。

重点说一下成员 st_mode，该成员描述了文件的`类型`和`权限`两属性。st_mode 是个32位的整形变量，不过现在的 linux 操作系统只用了低16位。st_mode 主要包括3部分内容：

```
1. 0-8：保存文件访问权限
    0-2: other权限
    3-5: group权限
    6-8: user权限
2. 9-11: 特殊权限
3. 12-15: 文件类型
```

st_mode 的占位情况如下：

![](images/Snipaste_2022-08-28_18-17-56.png)

<font color=red size=5>文件类型(File type):</font>
  
先看一下文件属性区域，位于 bit12~bit15，在现代 linux 操作系统上文件类型分为7种，分别是：

> 普通文件 (regular file)
> 目录 (directory)
> 字符设备 (character device)
> 块设备 (block device)
> 管道 (filo)
> 符号链接文件 (symbolic link)
> 套接字文件 (socket)

所以文件属性只需要 3bit 就够了，估计考虑到以后的扩展，所以这里保留了4位。在 <sys/stat.h> 中有如下宏定义：

```c
#define S_IFMT  00170000
#define S_IFSOCK 0140000
#define S_IFLNK  0120000
#define S_IFREG  0100000
#define S_IFBLK  0060000
#define S_IFDIR  0040000
#define S_IFCHR  0020000
#define S_IFIFO  0010000
#define S_ISUID  0004000    //文件的(set user-id on execution)位
#define S_ISGID  0002000    //文件的(set group-id on execution)位
#define S_ISVTX  0001000    //文件的sticky位
 
#define S_ISLNK(m)      (((m) & S_IFMT) == S_IFLNK) //判断是否为符号链接
#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG) //判断是否为一般文件
#define S_ISDIR(m)      (((m) & S_IFMT) == S_IFDIR) //判断是否为目录
#define S_ISCHR(m)      (((m) & S_IFMT) == S_IFCHR) //判断是否为字符设备
#define S_ISBLK(m)      (((m) & S_IFMT) == S_IFBLK) //判断是否为块设备
#define S_ISFIFO(m)     (((m) & S_IFMT) == S_IFIFO) //判断是否为FIFO文件
#define S_ISSOCK(m)     (((m) & S_IFMT) == S_IFSOCK) //判断是否为socket文件
```

首先 S_IFMT 是一个掩码，它的值是 0170000 (注意这里用的是八进制)，可以用来过滤出前4位表示的文件类型。

现在假如我们要判断一个文件是不是目录，可以使用如下方式：

```c
if ((info.st_mode & S_IFMT) == S_IFDIR)
    printf("this is a directory");

//或者直接使用<sys/stat.h>中定义的宏
if (S_ISDIR(info.st_mode))
    printf("this is a directory");
```

<font color=red size=5>文件权限(Permissions):</font>

接着来看 Permissions 区域的 bit0~bit8，也即 st_mode 字段的最低9位，代表文件的许可权限，它标识了文件所有者(owner)、组用户(group)、其他用户(other)的读(r)、写(w)、执行(x)权限。在 <sys/stat.h>中有如下宏定义：

```c
#define S_IRWXU 00700	/* mask for file owner permissions */
#define S_IRUSR 00400	/* owner has read permission */
#define S_IWUSR 00200	/* owner has write permission */
#define S_IXUSR 00100	/* owner has execute permission */
 
#define S_IRWXG 00070	/* mask for group permissions */
#define S_IRGRP 00040	/* group has read permission */
#define S_IWGRP 00020	/* group has write permission */
#define S_IXGRP 00010	/* group has execute permission */
 
#define S_IRWXO 00007	/* mask for permissions for others (not in group) */
#define S_IROTH 00004	/* others have read permission */
#define S_IWOTH 00002	/* others have write permission */
#define S_IXOTH 00001	/* others have execute permission */
```

程序中可以自由组合使用它们。

值得一提的是，目录的权限与普通文件的权限是不同的。目录的读、写、执行权限含义分别如下：

1. 读权限。读权限允许我们通过 opendir() 函数读取目录，进而可以通过 opendir() 函数获得目录内容，即目录下的文件列表 。
2. 写权限。写权限代表的是可在目录内创建、删除文件，而不是批的写目录本身。
3. 执行权限。可访问目录中的文件。


强调一下特殊权限位 SBIT(粘滞位)的功能：

1. 对目录设置粘滞位，则该目录内的文件只能被文件所有者、超级用户和目录所有者这三类用户删除，其他用户都没有删除权限;
2. 对文件设置了粘滞位，那么在内存资源十分紧张的情况下，也不会把该文件放回磁盘上。如磁盘的交换区 swap，当内存紧张，优先级别低的进程会被暂时放回到交换区，但是一旦设置了粘滞位，则不会放回磁盘，依然处于内存。

这里有一个例子，使用 stat 函数实现 `ls -l` 命令的功能。其运行结果如下图：(源文件见 stat.c)

![](images/Snipaste_2022-08-28_21-14-33.png)

