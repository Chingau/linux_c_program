/****************************************************************
file:         dev_rw.h
description:  the source file of device read
              and write api definition
****************************************************************/
#ifndef __DEV_RW_H__
#define __DEV_RW_H__

#include "moduleid_def.h"

typedef enum DEV_RW_ERROR_CODE
{
    DEV_RW_INVALID_PARAMETER = (MPU_MID_MID_DEV_RW << 16) | 0x01,
    DEV_RW_READ_FAILED,
    DEV_RW_WRITE_FAILED,
} DEV_RW_ERROR_CODE;

int dev_read(int fd, unsigned char *buf, unsigned int len);
int dev_write(int fd, unsigned char *buf, unsigned int len);

#endif

