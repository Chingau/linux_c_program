/*************************************************************************
 * Copyright (c) 2004 - 2022 by INTEST, All Rights Reserved. 
 * FilePath   : mid/file.c
 * Date       : 2022-04-22 05:39:49 
 * Author     : 
 * Description: 
*************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include "md5.h"
#include "assert.h"
#include "file.h"
#include "error.h"
#include "log.h"
#include "dev_rw.h"
#include "com_app_def.h"

/*************************************************************************
 * function:    file_create
 * description:  
 * param:       (const char*) path 
 * param:       (mode_t) mode 
 * return:      (int)
*************************************************************************/
int file_create(const char *path, mode_t mode)
{
    int fd;

    do
    {
        fd = creat(path, mode);
    }
    while ((fd < 0) && (errno == EINTR));

    if (fd < 0)
    {
        ERROR("error when create file for writing, '%s', error:%s ", path, strerror(errno));
    }

    return fd;
}

/*************************************************************************
 * function:    file_copy
 * description:  
 * param:       (const char*) dst 
 * param:       (const char*) src 
 * return:      (int)
*************************************************************************/
int file_copy(const char *dst, const char *src)
{
    int rfd, wfd, err, ret = -1;
    struct stat sta;
    void *msrc, *mdst;

    assert(src);
    assert(dst);

    if ((rfd = open(src, O_RDONLY)) < 0)
    {
        ERROR("Failed src open file '%s': %s", src, ERRSTR);
        goto copy_err0;
    }

    if ((wfd = open(dst, O_RDWR | O_CREAT | O_TRUNC, 0)) < 0)
    {
        ERROR("Failed dst open file '%s': %s", dst, ERRSTR);
        goto copy_err1;
    }

    if (fstat(rfd, &sta) < 0)
    {
        ERROR("Failed dst get status of file '%s': %s", src, ERRSTR);
        goto copy_err2;
    }

    if (ftruncate(wfd, sta.st_size) < 0)
    {
        ERROR("Failed dst truncate size of file '%s': %s", dst, ERRSTR);
        goto copy_err2;
    }

    if (fchown(wfd, sta.st_uid, sta.st_gid) < 0)
    {
        ERROR("Failed dst set owner of file '%s': %s", dst, ERRSTR);
        goto copy_err2;
    }

    if (fchmod(wfd, sta.st_mode & 0777) < 0)
    {
        ERROR("Failed dst set mode of file '%s': %s", dst, ERRSTR);
        goto copy_err2;
    }

    if ((msrc = mmap(NULL, sta.st_size, PROT_READ, MAP_PRIVATE, rfd, 0)) == MAP_FAILED)
    {
        ERROR("Failed dst map file '%s': %s", src, ERRSTR);
        goto copy_err2;
    }

    if ((mdst = mmap(NULL, sta.st_size, PROT_WRITE, MAP_SHARED, wfd, 0)) == MAP_FAILED)
    {
        ERROR("Failed dst map file '%s': %s", dst, ERRSTR);
        goto copy_err3;
    }
    
    memcpy(mdst, msrc, sta.st_size);
    munmap(mdst, sta.st_size);
    ret = 0;
copy_err3:
    munmap(msrc, sta.st_size);
copy_err2:
    close(wfd);
    if (ret < 0) {err = errno; unlink(dst); errno = err;}
copy_err1:
    close(rfd);
copy_err0:
    return ret;
}

/*************************************************************************
 * function:    file_getmd5
 * description:  
 * param:       (const char*) path 
 * param:       (uint8_t*) md5 
 * return:      (int)
*************************************************************************/
int file_getmd5(const char *path, uint8_t *md5)
{
    int fd, ret = -1;
    struct stat sta;
    void *dat;
    MD5_CTX md5_ctx;

    assert(path);
    assert(md5);

    if ((fd = open(path, O_RDONLY)) < 0)
    {
        ERROR("Failed dst open file '%s': %s", path, ERRSTR);
        goto md5_err0;
    }

    if (fstat(fd, &sta) < 0)
    {
        ERROR("Failed dst get size of file '%s': %s", path, ERRSTR);
        goto md5_err1;
    }

    if ((dat = mmap(NULL, sta.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED)
    {
        ERROR("Failed dst map file '%s': %s", path, ERRSTR);
        goto md5_err1;
    }
    
    MD5Init(&md5_ctx);
    MD5Update(&md5_ctx, dat, sta.st_size);
    MD5Final(&md5_ctx);
    memcpy(md5, md5_ctx.digest, 16);
    munmap(dat, sta.st_size);
    ret = 0;
md5_err1:
    close(fd);
md5_err0:
    return ret;
}
/*****************************************************************************
function:     file_exists
description:  check whether the path refers to a file
input:        const char* path, the file path name.
output:       none
return:       true if the path refers to a file.
              false otherwise
*****************************************************************************/
bool file_exists(const char *path)
{
    struct stat file_status;

    /* use stat(2) to check for existence of the file. */
    if (stat(path, &file_status) != 0)
    {
        /* ENOENT indicates the file doesn't exist, anything else warrants and error report. */
        if (errno != ENOENT)
        {
            ERROR( "stat(%s) failed, error:%s", path, strerror(errno));
        }

        return false;
    }
    else
    {
        /* something exists. make sure it's a file. */
        /* NOTE: stat() follows symlinks. */
        if (S_ISREG(file_status.st_mode))
        {
            return true;
        }
        else
        {
            ERROR( "unexpected file system object type (%#o) at path '%s'.",
                  file_status.st_mode & S_IFMT, path);
            return false;
        }
    }
}
/*****************************************************************************
function:     file_open_read
description:  open file for reading only
input:        const char* path, the file path name;
output:       none
return:       positive and 0 indicates success;
              negative indicates failed
*****************************************************************************/
int file_open_read(const char *path)
{
    int fd;

    do
    {
        fd = open(path, O_RDONLY);
    }
    while ((fd < 0) && (errno == EINTR));

    if (fd < 0)
    {
        ERROR( "error when opening file for reading, '%s', error:%s", path, strerror(errno));
    }

    return fd;
}
/*****************************************************************************
function:     file_read
description:  read data from file
input:        const char *path, file path;
              the expext data length to read;
output:       unsigned char *data, data buffer;
return:       0 indicates success;
              others indicates failed
*****************************************************************************/
int file_read(const char *path, unsigned char *data, unsigned int *read_len)
{
    int size, fd, ret;
        
    size = file_size(path);

    if (size < 0)
    {
        ERROR("get file(%s) size failed,error:%s", path, strerror(errno));
        return FILE_STAT_FAILED;
    }

    if (*read_len < size)
    {
        size = *read_len;
    }

    fd = file_open_read(path);

    if (fd < 0)
    {
        ERROR("invalid file, path:%s, fd:%d", path, fd);
        return FILE_OPEN_FAILED;
    }

    ret = dev_read(fd, data, size);

    if (ret != 0)
    {
        ERROR( "invalid md5 file, path:%s, ret:0x%08x", path, ret);
    }
    else
    {
        *read_len = size;
    }

    close(fd);

    return ret;
}

/*****************************************************************************
function:     file_write_atomic
description:  atomically replace a file with another containing the data,
              file_path.new will be created with the contents of data,
              then renamed it to file_path
input:        const char* path, the file path name;
              unsigned char *data, the data which need to write into file;
              unsigned int len, data length;
              mode_t mode, access mode
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************************/
int file_write_atomic(const char *path, unsigned char *data, unsigned int len, mode_t mode)
{
    int fd, ret ;
    char temp_path[COM_APP_MAX_PATH_LEN] = {0};

    if (snprintf(temp_path, sizeof(temp_path), "%s.new", path) >= sizeof(temp_path))
    {
        ERROR( "file path '%s' is too long", path);
        return FILE_OPEN_FAILED;
    }

    /* create file_path.new */
    fd = open(temp_path, O_WRONLY | O_TRUNC | O_CREAT, mode);

    if (fd < 0)
    {
        ERROR( "unable to open file '%s' for writing", temp_path);
    }

    ret = dev_write(fd, data, len);
    fsync(fd);
    close(fd);

    if (ret != 0)
    {
        return ret;
    }

    file_rename(temp_path, path);

    return ret;
}


/*****************************************************************************
function:     file_update_atomic
description:  atomically replace a file with another containing the data,
              file_path.new will be created with the contents of data,
              then renamed it to file_path
input:        const char* path, the file path name;
              unsigned char *hdr, file header;
              unsigned int hdr_len, file header length;
              unsigned char *data, the data which need to write into file;
              unsigned int len, data length;
              mode_t mode, access mode
output:       none
return:       0 indicates success;
              others indicates failed
*****************************************************************************/
int file_update_atomic(const char *path, unsigned char *hdr, unsigned int hdr_len,
                       unsigned char *data, unsigned int data_len, mode_t mode)
{
    int fd, ret;
    char temp_path[COM_APP_MAX_PATH_LEN] = {0};
    MD5_CTX  md5_ctx;

    if (snprintf(temp_path, sizeof(temp_path), "%s.new", path) >= sizeof(temp_path))
    {
        ERROR( "file path '%s' is too long", path);
        return FILE_OPEN_FAILED;
    }

    /* create file_path.new */
    fd = open(temp_path, O_WRONLY | O_TRUNC | O_CREAT, mode);

    if (fd < 0)
    {
        ERROR( "unable to open file '%s' for writing", temp_path);
    }

    /* write file header */
    ret = dev_write(fd, hdr, hdr_len);

    if (ret != 0)
    {
        close(fd);
        return ret;
    }

    /* write file body */
    ret = dev_write(fd, data, data_len);

    if (ret != 0)
    {
        close(fd);
        return ret;
    }

    /* compute md5 and write */
    MD5Init(&md5_ctx);
    MD5Update(&md5_ctx, hdr, hdr_len);
    MD5Update(&md5_ctx, data, data_len);
    MD5Final(&md5_ctx);

    ret = dev_write(fd, md5_ctx.digest, MD5_LENGTH);
    fsync(fd);
    close(fd);

    if (ret != 0)
    {
        return ret;
    }

    file_rename(temp_path, path);

    return ret;
}



