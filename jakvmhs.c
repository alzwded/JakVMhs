#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <dlfcn.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jakvmhs.h"

struct {
#define SP 31
#define IP 32
#define RLAST 33
    short regs[RLAST];
    unsigned char code[0x10000];
    short data[0x10000];

    short stack_data[0x10000];
    short call_stack[0x1000][RLAST]; // 4096 levels

    short(* csp)[RLAST];
} machine;

typedef enum {
    LS_UNDEFINED = 0,
    LS_FIRST,
    LS_SECOND
} logger_state_t;
static logger_state_t g_logger_state = LS_UNDEFINED;

#define cassert(X) (!(X) ? fprintf(stderr, "Assertion failed: %s\n", #X), abort(), 0 : 1)

static char const* g_image = NULL;
static int g_flags = ~0;

//============================================================
// internal
//============================================================
#define LOG_ERR 0x80000000

static inline void logger(int flags, char const* fmt, ...)
{
    FILE* f = stdout;
    int skip = 0;
    if(flags & LOG_ERR) f = stderr;
    if((flags & (0 | 0))) {
        if(!(flags & g_flags))
        {
            skip = 1;
        }
    }

    va_list args;
    va_start(args, fmt);
    if(!skip) vfprintf(f, fmt, args);
    va_end(args);
}

static void error(char const* msg)
{
    logger(LOG_ERR, "Error @%d %s\n", machine.regs[IP], (msg)?msg:"");
    fflush(stderr);
    abort();
}

static void reset_machine_state()
{
    memset(&machine.stack_data[0], 0, 0xFFFF * sizeof(short));
    memset(&machine.call_stack[0], 0, 0xFFF * RLAST * sizeof(short));
    machine.regs[SP] = 0;
    machine.csp = &machine.call_stack[0];
    machine.regs[IP] = 0;
}

static void load_image()
{
    logger(0, "Loading %s\n", g_image);

    int fd = open(g_image, O_RDONLY);
    cassert(fd != -1);

    struct stat sb;
    cassert(fstat(fd, &sb) != -1);

    off_t offset = 0;
    size_t length = sb.st_size;
    cassert(length >= 0x30000);
    if(length > 0x30000) logger(0, "WARNING: image bigger than the expected %ld bytes\n", 0x30000);

    char* image = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, offset);

    // copy memory
    memcpy(machine.data, image + 0x10000, sizeof(short) * 0x10000);
    memcpy(machine.code, image, sizeof(unsigned char) * 0x10000);

    munmap(image, sb.st_size);
    close(fd);
}

static void push(short x)
{
    cassert(machine.regs[SP] < 0x10000);
    machine.stack_data[machine.regs[SP]++] = x;
}

static short pop()
{
    cassert(machine.regs[SP] > 0);
    short ret = machine.stack_data[--machine.regs[SP]];
    return ret;
}

//=============================================================
// OS
//=============================================================

static char* os_deref_string(unsigned short w)
{
    error("os_deref_string NOT IMPLEMENTED");
}

static char const* os_from_short_name(unsigned short w)
{
    error("os_from_short_name NOT IMPLEMENTED");
}

static void os_logword()
{
    short w = pop();
    switch(g_logger_state) {
    case LS_SECOND:
        printf("%035X\n", (int)w);
        g_logger_state = LS_FIRST;
        break;
    case LS_FIRST:
        printf("%035X", (int)w);
        g_logger_state = LS_SECOND;
        break;
    default:
        error("undefined log_word state");
    }
}

static void os_logstring_p()
{
    short w = pop();
    char* s = os_deref_string(w);
    switch(g_logger_state) {
    case LS_SECOND:
        printf("%35s\n", s);
        g_logger_state = LS_FIRST;
        break;
    case LS_FIRST:
        printf("%35s", s);
        g_logger_state = LS_SECOND;
        break;
    default:
        error("undefined log_word state");
    }
    free(s);
}

static void os_logstring()
{
    short w = pop();
    char const* s = os_from_short_name(w);
    switch(g_logger_state) {
    case LS_SECOND:
        printf("%35s\n", s);
        g_logger_state = LS_FIRST;
        break;
    case LS_FIRST:
        printf("%35s", s);
        g_logger_state = LS_SECOND;
        break;
    default:
        error("undefined log_word state");
    }
}

static void os_exec_vm_code(unsigned short address)
{
    error("NOT IMPLEMENTED os_exec_vm_code");
}

static unsigned short* os_deref(unsigned short address)
{
    return &machine.data[address];
}

static vm_utilities_t os_get_vm_utilities()
{
    vm_utilities_t utils;
    utils.pop = &pop;
    utils.push = &push;
    utils.deref_string = &os_deref_string;
    utils.from_short_name = &os_from_short_name;
    utils.exec_vm_code = &os_exec_vm_code;
    utils.deref = &os_deref;
    return utils;
}

static void os_callextroutine()
{
    short wLib = pop();
    short wFunc = pop();
    char const* libname = os_from_short_name(wLib);

    typedef struct {
        char* libname;
        utility_lib_t utilities;
    } type_t;
    static type_t loadedUtilities[128];
    static size_t numLoadedUtilities;

    // find so libname
    size_t i = 0;
    int found = -1;
    for(; i < numLoadedUtilities; ++i) {
        if(strcmp(libname, loadedUtilities[i].libname) == 0) {
            cassert(wFunc < loadedUtilities[i].utilities.numUtilities);
            found = i;
            break;
        }
    }
    if(found == -1) {
        char actualLibName[256];
        sprintf(actualLibName, "lib%s.so", libname);

        void* dll = dlopen(actualLibName, RTLD_LAZY);

        if(!dll) error("failed to load library");
        utility_lib_t (*initialize)(void);

        *(void**) (&initialize) = dlsym(dll, "initialize");
        if(dlerror()) error("failed to call initialize");

        loadedUtilities[numLoadedUtilities].libname = (char*)malloc(strlen(libname));
        strcpy(loadedUtilities[numLoadedUtilities].libname, libname);
        loadedUtilities[numLoadedUtilities].utilities = (*initialize)();
        found = numLoadedUtilities;
        numLoadedUtilities++;
    }
    loadedUtilities[found].utilities.utilities[wFunc](os_get_vm_utilities(), &machine.regs);
    // find proc 
}

//============================================================
// operations
//============================================================

static void interrupt()
{
    short which = pop();
    switch(which) {
    case 1:
        error("NOT IMPLEMENTED: assign_short_name");
        break;
    case 2:
        error("NOT IMPLEMENTED: free_short_name");
        break;
    case 3:
        os_logword();
        break;
    case 4:
        os_logstring();
        break;
    case 5:
        os_logstring_p();
        break;
    case 6:
        error("NOT IMPLEMENTED: read_word");
        break;
    case 7:
        error("NOT IMPLEMENTED: read_string");
        break;
    case 8:
        error("NOT IMPLEMENTED: deref_short_name");
        break;
    case 20:
        os_callextroutine();
        break;
    case 0:
        logger(LOG_ERR, "Utility 0 is reserved and undefined\n");
        /*FALLTHROUGH*/
    default:
        error("Undefined utility called");
    }
}

static void add()
{
    short b = pop();
    short a = pop();
    push(a + b);
}

static void and()
{
    unsigned short b = pop();
    unsigned short a = pop();
    push(a & b);
}

static void compare()
{
    short b = pop();
    short a = pop();

    push(b > a);
}

static void compare_unsigned()
{
    unsigned short b = pop();
    unsigned short a = pop();

    push(b > a);
}

static void compare_signed()
{
    short b = pop();
    short a = pop();

    push(b > a);
}

static void div_op()
{
    short b = pop();
    short a = pop();
    push(a / b);
}

static void ior()
{
    unsigned short b = pop();
    unsigned short a = pop();
    push(a | b);
}

static void jump()
{
    unsigned short addr = pop();
    machine.regs[IP] = addr;
}

static void jump_ifzero()
{
    unsigned short addr = pop();
    short cond = pop();

    if(!cond) machine.regs[IP] = addr;
}

static void load()
{
    unsigned short addr = pop();
    push(machine.data[addr]);
}

static void mul()
{
    push(pop() * pop());
}

static void mod()
{
    short b = pop();
    short a = pop();
    push(a % b);
}

static void neg()
{
    unsigned short a = pop();
    push(~a);
}

static void not()
{
    push(!pop());
}

static void pop_op()
{
    (void) pop();
}

static void pop_register(size_t reg)
{
    machine.regs[reg] = pop();
}

static void push_immed()
{
    unsigned short addr = machine.regs[IP];
    unsigned char hi = machine.code[addr + 1],
                  lo = machine.code[addr + 2];
    push((hi << 8) | lo);
    machine.regs[IP] += 2;
}

static void reset()
{
    reset_machine_state();
    load_image();
}

static void register_dec(size_t reg)
{
    machine.regs[reg]--;
}

static void register_inc(size_t reg)
{
    machine.regs[reg]++;
}

static void register_mask(size_t reg)
{
    unsigned short mask = pop();
    unsigned short val = machine.regs[reg];
    push(mask & val);
}

static void register_push(size_t reg)
{
    push(machine.regs[reg]);
}

static void register_rol(size_t reg)
{
    short x = pop();
    int left = x > 0;
    short amount = x & 0x1F;
    unsigned short v = machine.regs[reg];
    unsigned short mask = 0xFFFF;
    if(left) {
        mask <<= amount;
        mask >>= amount;
        mask = ~mask;
        machine.regs[reg] = ((mask & v) >> amount) | v << amount;
    } else {
        mask >>= amount;
        mask <<= amount;
        mask = ~mask;
        machine.regs[reg] = ((mask & v) << amount) | v >> amount;
    }
}

static void register_sh(size_t reg)
{
    short x = pop();
    int left = x > 0;
    short amount = x & 0x1F;
    if(left) {
        machine.regs[reg] <<= amount;
    } else {
        machine.regs[reg] >>= amount;
    }
}

static void return_op()
{
    cassert(machine.csp > 0);
    memcpy(&machine.regs[0], --machine.csp, sizeof(short) * RLAST);
}

static void save()
{
    cassert(machine.csp - machine.call_stack < 0xFFF);
    memcpy(machine.csp++, &machine.regs[0], sizeof(short) * RLAST);
    machine.csp[-1][IP]++; // increment return address
}

static void store()
{
    short val = pop();
    unsigned short addr = pop();
    machine.data[addr] = val;
}

static void sub()
{
    short b = pop();
    short a = pop();
    push(a - b);
}

static void xor()
{
    unsigned short b = pop();
    unsigned short a = pop();
    push(a ^ b);
}

//============================================================
// decoder
//============================================================

static void further_decode()
{
    unsigned char addr = machine.regs[IP];
    unsigned char opcode = machine.code[addr];
    switch(opcode) {
        default:
        case 0x00:
            break;
        case 0x01:
            interrupt();
            break;
        case 0x02:
            reset();
            break;
        case 0x05:
            push_immed();
            break;
        case 0x06:
            save();
            break;
        case 0x07:
            return_op();
            break;
        case 0x08:
            load();
            break;
        case 0x09:
            store();
            break;
        case 0x0A:
            add();
            break;
        case 0x0B:
            sub();
            break;
        case 0x0C:
            mul();
            break;
        case 0x0D:
            mod();
            break;
        case 0x0E:
            div_op();
            break;
        case 0x10:
            and();
            break;
        case 0x11:
            ior();
            break;
        case 0x12:
            xor();
            break;
        case 0x13:
            not();
            break;
        case 0x17:
            neg();
            break;
        case 0x18:
            compare_signed();
            break;
        case 0x19:
            compare_unsigned();
            break;
        case 0x1E:
            jump();
            break;
        case 0x1F:
            jump_ifzero();
            break;
    }
}

static void decode()
{
    unsigned char addr = machine.regs[IP];
    unsigned char opcode = machine.code[addr];
    switch((opcode >> 5) & 0x7) {
        default:
        case 0x0:
            further_decode();
            break;
        case 0x1:
            register_mask(opcode & 0x1F);
            break;
        case 0x2:
            register_sh(opcode & 0x1F);
            break;
        case 0x3:
            register_rol(opcode & 0x1F);
            break;
        case 0x4:
            register_push(opcode & 0x1F);
            break;
        case 0x5:
            pop_register(opcode & 0x1F);
            break;
        case 0x6:
            register_inc(opcode & 0x1F);
            break;
        case 0x7:
            register_dec(opcode & 0x1F);
            break;
    }
}

void exec()
{
    while(1) {
        decode();
        machine.regs[IP]++;
    }
}

//============================================================
// main
//============================================================

int main(int argc, char* argv[])
{
    if(argc != 2) abort();
    memset(&machine.regs[0], 0, sizeof(short) * RLAST);
    g_image = argv[1];

    g_logger_state = LS_FIRST;

    reset_machine_state();
    load_image();

    exec();
    return 0;
}
