#ifndef __TEST_API_H__
#define __TEST_API_H__

typedef enum TEST_NAME {
    TEST_NAME_0 = 0,
    TEST_NAME_1,
    TEST_NAME_2,
    TEST_NAME_3,
    TEST_NAME_4,
} TEST_NAME;

int test_init(int phase);
int test_run(void);

#endif
