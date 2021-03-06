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
#include <stdbool.h>

#include "jakvmhs.h"

struct {
#define RA 30
#define SP 31
#define IP 32
#define RLAST 33
    signed_t regs[RLAST];
    code_t code[0x10000];
    signed_t data[0x10000];

    signed_t stack_data[0x10000];
} machine;

#define cassert(X) (!(X) ? fprintf(stderr, "Assertion failed at %s:%d in %s:\n\t%s\n", __FILE__, __LINE__, __func__, #X), exit(42), 0 : 1)

static char const* g_image = NULL; // executable image filename
static signed_t* g_save_data = NULL; // pointer to mmap'd region

//============================================================
// internal
//============================================================
typedef enum {
    LS_UNDEFINED = 0,
    LS_FIRST,
    LS_SECOND
} logger_state_t;
static logger_state_t g_logger_state = LS_UNDEFINED;

#define LOG_ERR 0x80000000  // log to stderr instead of stdout
#define LOG_SAVEFILE 0x1    // enable logging SAVEFILE related msgs
static int g_flags = ~0; // logger flags

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

static void usage(char const* imgname)
{
    printf("Usage: %s image.hss\n", imgname);
    exit(255);
}

static void error(char const* msg)
{
    logger(LOG_ERR, "Error @%d %s\n", machine.regs[IP], (msg)?msg:"");
    fflush(stderr);
    exit(42);
}

//-------------------------------------------------------------
// loaders
//-------------------------------------------------------------

// execute reset action
static void reset_machine_state()
{
    // clear stacks
    memset(&machine.stack_data[0], 0, 0xFFFF * sizeof(signed_t));
    machine.regs[SP] = 0;
    // start at 0x0
    machine.regs[IP] = 0;
}

// loads or creates the persistent file and mmaps it into g_save_data
// the filename is essentially [csh] g_image:r.sav
static signed_t* open_save_data()
{
    // determine file name
    char* rName = (char*)malloc(strlen(g_image) + 4);
    char* p = strrchr(g_image, '.');
    if(p) {
        (void) strncpy(rName, g_image, p - g_image);
        rName[p - g_image] = '\0';
    } else {
        (void) strcpy(rName, g_image);
    }
    (void) strcat(rName, ".sav");

    // does file exist?
    int fd = open(rName,
            O_CREAT | O_RDWR,
            S_IRUSR|S_IWUSR);
    cassert(fd != -1);

    struct stat sb;
    cassert(fstat(fd, &sb) == 0);

    off_t len = sb.st_size;
    size_t expectedLen = 512u;
    if(len != expectedLen) {
        logger(LOG_SAVEFILE|LOG_ERR, "File %s is of unexpected size, truncating and nullifying...\n", rName);
        ftruncate(fd, expectedLen);
    }

    // map the save file
    void* ptr = mmap(
            NULL,
            expectedLen,
            PROT_READ|PROT_WRITE,
            MAP_SHARED,
            fd,
            0);
    cassert(ptr);

    close(fd);

    return (signed_t*)ptr;
}

// release mmap'd region
static void dispose_of_save_data(signed_t** p)
{
    cassert(p);
    munmap(*p, 512);
    *p = NULL;
}

// get the pointer to the mmap'd region of the persistent file
static signed_t* get_save_data_ptr()
{
    if(g_save_data) return g_save_data;
    return g_save_data = open_save_data();
}

// load the executable image
static void load_image()
{
    cassert(g_image);
    if(g_save_data) dispose_of_save_data(&g_save_data);

    logger(0, "Loading %s\n", g_image);

    int fd = open(g_image, O_RDONLY);
    cassert(fd != -1);

    struct stat sb;
    cassert(fstat(fd, &sb) != -1);

    off_t offset = 0;
    size_t length = sb.st_size;
    cassert(length >= 0x30000);
    if(length > 0x30000) logger(LOG_ERR, "WARNING: image bigger than the expected %ld bytes\n", 0x30000);

    char* image = mmap(NULL, length, PROT_READ, MAP_PRIVATE, fd, offset);

    // copy memory
    memcpy(machine.data, image + 0x10000, sizeof(signed_t) * 0x10000);
    memcpy(machine.code, image, sizeof(code_t) * 0x10000);

    munmap(image, sb.st_size);
    close(fd);
}

//-------------------------------------------------------------
// stack management
//-------------------------------------------------------------

static void push(signed_t x)
{
    cassert(machine.regs[SP] < 0x10000);
    machine.stack_data[machine.regs[SP]++] = x;
}

static signed_t pop()
{
    cassert(machine.regs[SP] > 0);
    signed_t ret = machine.stack_data[--machine.regs[SP]];
    return ret;
}

//=============================================================
// OS
//=============================================================

//-------------------------------------------------------------
// OS.SN
//-------------------------------------------------------------

// dereference an internal string as a C string
// must be free'd
static char* os_deref_string(unsigned_t pStr)
{
    size_t start = pStr;
    size_t end;
    for(end = start; ; ++end) {
        if((machine.data[end] & 0xFF00) == 0)
        {
            break;
        }
    }

    size_t count = start, len = 0;
    char* p = (char*)&machine.data[start];
    char* decoded = (char*)malloc(sizeof(char) * (end - start + 1));
    while(1) {
        unsigned_t crnt = machine.data[count];
        decoded[len++] = (crnt & 0xFF00) >> 8;
        if(!decoded[len - 1]) break;
        ++count;
        cassert(count < 0x10000);
    }

    char* rets = (char*)malloc(sizeof(char) * strlen(decoded));
    strcpy(rets, decoded);
    free(decoded);

    return rets;
}

//-------------------------------------------------------------
// OS.log
//-------------------------------------------------------------

// log a single word
static void os_logword()
{
    signed_t w = pop();
    switch(g_logger_state) {
    case LS_SECOND:
        printf("%35X\n", (int)w);
        g_logger_state = LS_FIRST;
        break;
    case LS_FIRST:
        printf("%35X", (int)w);
        g_logger_state = LS_SECOND;
        break;
    default:
        error("undefined log_word state");
    }
}

// log a null terminated memory location
static void os_logstring_p()
{
    unsigned_t w = pop();
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

//-------------------------------------------------------------
// OS.persistent
//-------------------------------------------------------------

// read a single word from persistent storage
static void os_read_save_word()
{
    signed_t save_word_address = pop();
    cassert(save_word_address >= 0 && save_word_address < 256);

    signed_t* save_data = get_save_data_ptr();
    cassert(save_data);
    push(save_data[save_word_address]);
}

// write a single word from persistent storage
static void os_write_save_word()
{
    signed_t save_word_address = pop();
    signed_t word = pop();
    cassert(save_word_address >= 0 && save_word_address < 256);

    signed_t* save_data = get_save_data_ptr();
    cassert(save_data);
    save_data[save_word_address] = word;
}

// transfer N words from persistent storage into memory
static void os_get_save_data()
{
    signed_t save_data_addr = pop();
    unsigned_t howMuch = pop();
    unsigned_t mem_addr = pop();

    cassert(save_data_addr >= 0 && save_data_addr < 256);
    cassert((size_t)mem_addr + (size_t)howMuch < 0x10000);
    cassert((size_t)save_data_addr + (size_t)howMuch < 256);

    signed_t* save_data = get_save_data_ptr();

    memcpy(&machine.data[mem_addr], &save_data[save_data_addr], howMuch * sizeof(signed_t));
}

// transfer N words from memory to persistent storage
static void os_put_save_data()
{
    signed_t save_data_addr = pop();
    unsigned_t howMuch = pop();
    unsigned_t mem_addr = pop();

    cassert(mem_addr >= 0 && mem_addr < 256);
    cassert((size_t)save_data_addr + (size_t)howMuch < 0x10000);
    cassert((size_t)mem_addr + (size_t)howMuch < 256);

    signed_t* save_data = get_save_data_ptr();

    memcpy(&save_data[save_data_addr], &machine.data[mem_addr], howMuch * sizeof(signed_t));
}

//-------------------------------------------------------------
// OS.interop
//-------------------------------------------------------------

// R/W memory access
static unsigned_t* os_deref(unsigned_t address)
{
    return &machine.data[address];
}

// called from a utility library to start executing VM code from address
static void os_exec_vm_code(unsigned_t address)
{
    error("NOT IMPLEMENTED os_exec_vm_code");
}

// factory method for vm_utilities passed to utility libraries
static vm_utilities_t os_get_vm_utilities()
{
    vm_utilities_t utils;
    utils.pop = &pop;
    utils.push = &push;
    utils.deref_string = &os_deref_string;
    utils.exec_vm_code = &os_exec_vm_code;
    utils.deref = &os_deref;
    return utils;
}

// call an arbitrary utility routine from an arbitrary utility library
static void os_callextroutine()
{
    unsigned_t wLib = pop();
    unsigned_t wFunc = pop();
    char const* libname = os_deref_string(wLib);

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
    // load lib if not found || error
    if(found == -1) {
        char actualLibName[256];
        void* dll = NULL;

        // #1 attempt LD_LIBRARY_PATH
        sprintf(actualLibName, "lib%s.so", libname);
        dll = dlopen(actualLibName, RTLD_LAZY);
        if(!dll) {
            // #2 attempt current directory
            sprintf(actualLibName, "./lib%s.so", libname);
            dll = dlopen(actualLibName, RTLD_LAZY);
        }

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
    // exec proc
    loadedUtilities[found].utilities.utilities[wFunc](os_get_vm_utilities(), &machine.regs);
}

//============================================================
// operations
//============================================================

static void add()
{
    signed_t b = pop();
    signed_t a = pop();
    push(a + b);
}

static void and()
{
    unsigned_t b = pop();
    unsigned_t a = pop();
    push(a & b);
}

static void compare()
{
    signed_t b = pop();
    signed_t a = pop();

    push(b > a);
}

static void compare_unsigned()
{
    unsigned_t b = pop();
    unsigned_t a = pop();

    push(b > a);
}

static void compare_signed()
{
    signed_t b = pop();
    signed_t a = pop();

    push(b > a);
}

static void div_op()
{
    signed_t b = pop();
    signed_t a = pop();
    push((b) ? a / b : -32768);
}

static void halt_this_thing()
{
    printf("\n");
    exit(0);
}

static void interrupt()
{
    unsigned_t which = pop();
    switch(which) {
    case 3:
        os_logword();
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
    case 10:
        os_read_save_word();
        break;
    case 11:
        os_write_save_word();
        break;
    case 12:
        os_get_save_data();
        break;
    case 13:
        os_put_save_data();
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

static void ior()
{
    unsigned_t b = pop();
    unsigned_t a = pop();
    push(a | b);
}

static void jump()
{
    unsigned_t addr = pop();
    machine.regs[IP] = addr - 1;
}

static void jump_ifzero()
{
    unsigned_t addr = pop();
    signed_t cond = pop();

    if(!cond) machine.regs[IP] = addr - 1;
}

static void load()
{
    unsigned_t addr = pop();
    push(machine.data[addr]);
}

static void mod()
{
    signed_t b = pop();
    signed_t a = pop();
    push(a % b);
}

static void mul()
{
    push(pop() * pop());
}

static void neg()
{
    unsigned_t a = pop();
    push(~a);
}

static void not()
{
    push(!pop());
}

static void pop_register(size_t reg)
{
    machine.regs[reg] = pop();
}

static void dup_op()
{
    cassert(machine.regs[SP] > 0);
    signed_t val = machine.stack_data[machine.regs[SP] - 1];
    push(val);
}

static void push_immed()
{
    unsigned_t addr = machine.regs[IP];
    unsigned_t hi = machine.code[addr + 1],
               lo = machine.code[addr + 2];
    push((hi << 8) | lo);
    machine.regs[IP] += 2;
}

static void register_swap()
{
    unsigned_t tmp = machine.regs[0];
    machine.regs[0] = machine.regs[SP];
    machine.regs[SP] = tmp;
}

static void swap()
{
    unsigned_t n = pop();
    unsigned_t tmp = machine.stack_data[machine.regs[SP] - 1];
    machine.stack_data[machine.regs[SP] - 1] =
        machine.stack_data[machine.regs[SP] - 1 - n];
    machine.stack_data[machine.regs[SP] - 1 - n] = tmp;
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
    unsigned_t mask = pop();
    unsigned_t val = machine.regs[reg];
    push(mask & val);
}

static void register_push(size_t reg)
{
    push(machine.regs[reg]);
}

static void register_rol(size_t reg)
{
    signed_t x = pop();
    bool left = x > 0;
    unsigned_t amount = x & 0x1F;
    unsigned_t v = machine.regs[reg];
    unsigned_t mask = 0xFFFF;
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
    signed_t x = pop();
    bool left = x > 0;
    unsigned_t amount = x & 0x1F;
    if(left) {
        machine.regs[reg] <<= amount;
    } else {
        machine.regs[reg] >>= amount;
    }
}

static void return_op()
{
    unsigned_t addr = machine.regs[RA];
    machine.regs[IP] = addr;
}

static void call_op()
{
    unsigned_t addr = pop();
    machine.regs[RA] = machine.regs[IP];
    machine.regs[IP] = addr - 1;
}

static void store()
{
    signed_t val = pop();
    unsigned_t addr = pop();
    machine.data[addr] = val;
}

static void sub()
{
    signed_t b = pop();
    signed_t a = pop();
    push(a - b);
}

static void xor()
{
    unsigned_t b = pop();
    unsigned_t a = pop();
    push(a ^ b);
}

//============================================================
// decoder
//============================================================

static void further_decode()
{
    unsigned_t addr = machine.regs[IP];
    code_t opcode = machine.code[addr];
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
        case 0x03:
            dup_op();
            break;
        case 0x04:
            halt_this_thing();
            break;
        case 0x05:
            push_immed();
            break;
        case 0x06:
            call_op();
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
        case 0x0F:
            register_swap();
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
        case 0x14:
            swap();
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
    unsigned_t addr = machine.regs[IP];
    unsigned_t opcode = machine.code[addr];
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

//============================================================
// main
//============================================================

static void exec()
{
    while(1) {
        decode();
        machine.regs[IP]++;
    }
}

int main(int argc, char* argv[])
{
    if(argc < 2 || strcmp(argv[1], "-h") == 0) usage(argv[0]);
    cassert(argc == 2);
    memset(&machine.regs[0], 0, sizeof(signed_t) * RLAST);
    g_image = argv[1];

    g_logger_state = LS_FIRST;

    reset_machine_state();
    load_image();

    exec();
    return 0;
}
