#include <stdio.h>
#include <unistd.h>
#include "init.h"
#include "tcom_api.h"
#include "test_api.h"

typedef int (*module_init_fn)(int phase);
typedef int (*module_run_fn)(void);

static module_init_fn init_table[] = {
    tcom_init,
    test_init, //测试模块
};

static module_run_fn run_table[] = {
    tcom_run,
    test_run, //测试模块
};

int main(int argc, const char *argv[])
{
    int i, j;
    int ret = -1;

    for (j = 0; j < INIT_PHASE_COUNT; ++j) {
        for (i = 0; i < sizeof(init_table)/sizeof(module_init_fn); ++i) {
            ret = (init_table[i])(j);
            if (ret != 0) {
                printf("module(%u) init in phase(%u) failed, ret:%u.\r\n", i, j, ret);
                return -1;
            }
            printf("module(%u) init in phase(%u) successfully.\r\n", i, j);
        }
    }

    for (i = 0; i < sizeof(run_table)/sizeof(module_run_fn); ++i) {
        ret = (run_table[i])();
        if (ret != 0) {
            printf("module(%u) startup failed, ret:%u.\r\n", i, ret);
            return -1;
        }
        printf("module(%u) startup successfully.\r\n", i);
    }

    while (1) {
        sleep(0xffffffff);
    }

    return 0;
}
