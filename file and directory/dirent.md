
```c
struct dirent {
    ino_t d_ino;
    off_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[256];
}
```

**d_ino**:文件的inode号

**off_t**:

**d_reclen**:

**d_type**:该成员指明文件的类型，其取值如下：

|type|说明|
|:--|:--
DT_BLK|块设备
DT_CHR|字符设备
DT_DIR|目录
DT_FIFO|有名管道
DT_LNK|符号链接
DT_REG|普通文件，即ls -al显示出来的属性中，第一个属性为[-]，例如[-rwxrwxrwx]
DT_SOCK|UNIX domain socket
DT_UNKNOWN|未知

**d_name**:以NULL为结束符的文件名
