#include <stddef.h>
#include "jakvmhs.h"
#include <stdio.h>
#include <math.h>

static void test_printnum(vm_utilities_t vm, signed_t (*regs)[33])
{
    signed_t num = vm.pop();

    printf("Your number was: %d\n", (int)num);
}

static void test_pow(vm_utilities_t vm, signed_t (*regs)[33])
{
    signed_t exp = vm.pop();
    signed_t base = vm.pop();
    signed_t res = pow(base, exp);
    vm.push(res);
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
