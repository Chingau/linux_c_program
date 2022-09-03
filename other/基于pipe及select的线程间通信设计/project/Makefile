TARGET := app
CC := gcc
CFLAGS := -lpthread -lrt -Wall
#相对路径 $(shell pwd)绝对路径
ROOT_PATH := .

#获取所有的.c文件
SRC := $(wildcard $(ROOT_PATH)/*.c)
SRC += $(wildcard $(ROOT_PATH)/dev_rw/*.c)
SRC += $(wildcard $(ROOT_PATH)/tcom/*.c)
SRC += $(wildcard $(ROOT_PATH)/timer/*.c)
SRC += $(wildcard $(ROOT_PATH)/test/*.c)

#获取头文件路径
INC := -I$(ROOT_PATH)/include

OBJ := $(patsubst %.c, %.o, $(SRC))

$(TARGET):$(OBJ)
	$(CC) $(OBJ) -o $@

%.o:%.c
	$(CC) $(INC) -c $< -o $@ $(CFLAGS)


.PHONY: clean
clean:
	find $(ROOT_PATH) -name "*.o" | xargs rm -f
	find $(ROOT_PATH) -name "$(TARGET)" | xargs rm -f