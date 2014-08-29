#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct {
#define SP 31
#define IP 32
#define RLAST 33
    short regs[RLAST];
    unsigned char code[0xFFFF];
    short data[0xFFFF];

    short stack_data[0xFFFF];
    short call_stack[0xFFF][RLAST]; // 4096 levels

    short(* csp)[RLAST];
} machine;

#define cassert(X) (!(X) ? fprintf(stderr, "Assertion failed: %s\n", #X), abort(), 0 : 1)

char const* g_image = NULL;

//============================================================
// internal
//============================================================

static void load_image()
{
    memset(&machine.stack_data[0], 0, 0xFFFF * sizeof(short));
    memset(&machine.call_stack[0], 0, 0xFFF * RLAST * sizeof(short));
    machine.sp = 0;
    machine.csp = &machine.call_stack[0];
    machine.regs[IP] = 0;

    printf("loading %s\n", g_image);

    abort();
}

static void push(short x)
{
    cassert(machine.sp < 0xFFFF);
    machine.stack_data[machine.regs[SP]++] = x;
}

static short pop()
{
    cassert(machine.sp > 0);
    short ret = machine.stack_data[machine.regs[--SP]];
    return ret;
}

//============================================================
// operations
//============================================================

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

static void interrupt()
{
    abort();
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
    load_image();
    abort();
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
    load_image();
    exec();
    return 0;
}
