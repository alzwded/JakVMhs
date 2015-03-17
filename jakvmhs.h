#ifndef JAKVMHS_H
#define JAKVMHS_H

#include <stdint.h>

typedef int16_t signed_t;
typedef uint16_t unsigned_t;
typedef uint8_t code_t;

typedef struct {
    /* manipulate the VM stack (e.g. for grabbing parameters) */
    signed_t (*pop)();
    void (*push)(signed_t);

    /* start a procedure call into the VM; returns */
    void (*exec_vm_code)(unsigned_t address);
    /* dereference a pointer */
    unsigned_t* (*deref)(unsigned_t address);
    /* dereference a string pointer; needs to be free'd */
    char* (*deref_string)(unsigned_t address);
} vm_utilities_t;

typedef void (*utility_fn)(vm_utilities_t, signed_t (*regs)[33]);
typedef struct {
    size_t numUtilities;
    utility_fn* utilities;
} utility_lib_t;

#endif
