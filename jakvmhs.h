#ifndef JAKVMHS_H
#define JAKVMHS_H

typedef struct {
    /* manipulate the VM stack (e.g. for grabbing parameters) */
    short (*pop)();
    void (*push)(short);

    /* start a procedure call into the VM; returns */
    void (*exec_vm_code)(unsigned short address);
    /* dereference a pointer */
    unsigned short* (*deref)(unsigned short address);
    /* dereference a string pointer; needs to be free'd */
    char* (*deref_string)(unsigned short address);
} vm_utilities_t;

typedef void (*utility_fn)(vm_utilities_t, short (*regs)[33]);
typedef struct {
    size_t numUtilities;
    utility_fn* utilities;
} utility_lib_t;

#endif
