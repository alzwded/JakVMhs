#include <stddef.h>
#include "jakvmhs.h"
#include <stdio.h>
#include <math.h>

static void test_printnum(vm_utilities_t utils, short (*regs)[33])
{
    short num = utils.pop();

    printf("Your number was: %d\n", (int)num);
}

static void test_pow(vm_utilities_t utils, short (*regs)[33])
{
    short exp = utils.pop();
    short base = utils.pop();
    short res = pow(base, exp);
    utils.push(res);
}

utility_lib_t initialize()
{
    static utility_fn utils[] = {
        &test_printnum,
        &test_pow,
    };

    static utility_lib_t ret = {
        2,
        utils
    };

    return ret;
}
