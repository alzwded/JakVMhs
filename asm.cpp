#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <algorithm>
#include <cstdio>
#include <sys/types.h>
#include <cctype>
#include <cstdarg>

static enum asmmode_t {
    ERROR = 0,
    DATA,
    CODE
} mode = ERROR;

static FILE* fin = NULL,* fout = NULL;
static size_t data_pos, code_pos;
static size_t* current_size;
static int g_flags = 0xFFFFFFFF;

#define LOG_ERR 0x80000000
#define LOG_TOKENIZER 1
#define LOG_DATAGEN 2
#define LOG_LABELS 4

static inline void log(int flags, char const* fmt, ...)
{
    FILE* f = stdout;
    int skip = 0;
    if(flags & LOG_ERR) f = stderr;
    if((flags & (LOG_TOKENIZER | LOG_DATAGEN | LOG_LABELS))) {
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
    size_t pos = ftell(fin);
    log(LOG_ERR, "Error @%ld %s\n", pos, (msg)?msg:"");
    fflush(stderr);
    throw 1; // gracefully exit with an exception
}

//=============================================================
// internal
//=============================================================

#define cassert(X) (!(X) ? log(LOG_ERR, "Assertion failed: %s @%ld\n", #X, ftell(fin)), fflush(stderr), throw 0, 0 : 1)
#define END(X, N) if(X[N] != '\0') error("expected token to end");

static void clearOutputFile(FILE* f, size_t size)
{
    unsigned char buffer[0x1000];
    memset(buffer, 0, sizeof(unsigned char) * 0x1000);
    fseek(f, 0, SEEK_SET);
    for(size_t i = 0; i < size/0x1000; ++i) {
        fwrite(buffer, sizeof(unsigned char), 0x1000, f);
    }
    fseek(f, 0, SEEK_SET);
}

static void tillEol()
{
    int c;
    do {
        c = fgetc(fin);
        if(c == '\n') {
            ungetc(c, fin);
            return;
        }
        if(c == EOF || feof(fin)) {
            return;
        }
    } while(1);
}

static std::string getToken()
{
    char tok[128];
    char* p = &tok[0];
    int c;
    do {
        while(isspace(c = fgetc(fin)))
            ;
        if(c == ';') tillEol();
        else break;
    } while(1);
    *p++ = c;
    do {
        if(feof(fin)) break;

        c = fgetc(fin);

        if(c == EOF || feof(fin)) break;
        if(isspace(c)) {
            if((p - tok > 0)) break;
            else continue;
        }
        if(c == ',') {
            if(p - tok > 0 && tok[0] != '\'') {
                break;
            } else {
                continue;
            }
        }
        if(c == ';') {
            tillEol();
            continue;
        }

        *p++ = c;
    } while(1);
    *p++ = '\0';

    log(LOG_TOKENIZER, "next token: '%s'\n", tok);
    return tok;
}

std::map<std::string, size_t> label_definitions;
std::map<size_t, std::string> label_usages;

static void add_label_to_definitions(std::string const& token)
{
    log(LOG_LABELS, "label %s defined at %lX\n", token.c_str(), *current_size);
    auto labelAlreadyDefined = label_definitions.find(token);
    cassert(labelAlreadyDefined == label_definitions.end());
    unsigned short pos = 0;
    if(current_size == &data_pos) {
        pos = 0xFFFF & ((data_pos - 0x10000) / sizeof(short));
    } else {
        pos = 0xFFFF & *current_size;
    }
    label_definitions.insert(std::make_pair(std::string(token), pos));
}

static void add_label_used_at(size_t size, std::string const& token)
{
    log(LOG_LABELS, "label %s used at %lX\n", token.c_str(), *current_size);
    label_usages.insert(std::make_pair(size, std::string(token)));
}

static void resolve_labels()
{
    std::for_each(label_usages.begin(), label_usages.end(), [&](decltype(label_usages)::value_type const& lbl){
        auto found = label_definitions.find(lbl.second);
        if(found == label_definitions.end()) error("unknown label");
        unsigned short data = ((found->second >> 8) & 0xFF) | ((found->second & 0xFF) << 8);

        fseek(fout, lbl.first, SEEK_SET);
        fwrite(&data, sizeof(unsigned short), 1, fout);

        log(LOG_LABELS, "resolving %s to %X\n", lbl.second.c_str(), (int)data);
    });
}

//=============================================================
// output
//=============================================================

static void produce(unsigned char c)
{
    fwrite(&c, 1, 1, fout);
    *current_size = ftell(fout);
}


static void produce_reg(unsigned char code, char const* token)
{
    int nbr = -1;
    switch(token[3]) {
        case '1':
            switch(token[4]) {
            case '\0':
                nbr = 1;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                END(token, 5);
                nbr = 10 + (token[4] - '0');
                break;
            default: error("invalid register");
            }
            break;
        case '2':
            switch(token[4]) {
            case '\0':
                nbr = 2;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                END(token, 5);
                nbr = 20 + (token[4] - '0');
                break;
            default: error("invalid register");
            }
            break;
        case '3':
            switch(token[4]) {
            case '\0':
                nbr = 3;
                break;
            case '0':
            case '1':
                END(token, 5);
                nbr = 30 + (token[4] - '0');
                break;
            default: error("invalid register");
            }
            break;
        case '0':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
            END(token, 4);
            nbr = token[3] - '0';
            break;
        default: error("invalid register");
    }
    
    unsigned char reg = 0x1F & nbr;
    unsigned char opcode = code << 5;
    unsigned char i = opcode | reg;
    produce(i);
}


//=============================================================
// decoders
//=============================================================

#define switch_mode(X) do{ \
    if(X.compare(".code") == 0) { \
        mode = CODE; \
    } else if(X.compare(".data") == 0) { \
        mode = DATA; \
    } else { \
        mode = ERROR; \
    } \
}while(0)

static void for_data()
{
    while(!feof(fin)) {
        std::string name = getToken();
        if(name[0] == '.') {
            switch_mode(name);
            return;
        }
        add_label_to_definitions(name);

        std::string ssize = getToken();
        char* endptr;
        long size = strtol(ssize.c_str(), &endptr, 0);
        if(endptr && *endptr) error("invalid number");

        log(LOG_DATAGEN, "need to consume: %ld\n", size);

        while(size > 0 && !feof(fin)) {
            std::string name = getToken();
            if(name[0] == '\'') {
                size_t i = 1;
                auto pEnd = name.substr(1).rfind('\'');
                auto closingQuote = pEnd;
                cassert(closingQuote != std::string::npos);
                ++pEnd; // i starts at one
                while(i < pEnd) {
                    char c1 = '\0';
                    char c2 = '\0';

                    if(i < pEnd) {
                        c1 = name[i];
                    }

                    if(i < pEnd - 1) {
                        c2 = name[i + 1];
                    }

                    unsigned short data = ((unsigned char)c2) | ((unsigned char)c1 << 8);
                    fwrite(&data, sizeof(short), 1, fout);
                    *current_size = ftell(fout);
                    size--;
                    i += 2;
                    log(LOG_DATAGEN, "one byte down after %02X%02X\n", (int)c1, (int)c2);
                }
                log(LOG_DATAGEN, "after %s still need %ld\n", name.c_str(), size);
            } else {
                char* endptr;
                long num = strtol(name.c_str(), &endptr, 0);
                if(endptr && *endptr) error("invalid number");

                unsigned short data = num & 0xFFFF;
                fwrite(&data, sizeof(short), 1, fout);
                *current_size = ftell(fout);
                size--;
                log(LOG_DATAGEN, "after %s still need %ld\n", name.c_str(), size);
            }
        }
    }
}

static void push_imed()
{
    std::string token = getToken();
    produce(0x5);
    if(token[0] == ':') {
        add_label_used_at(*current_size, token);
        produce(0);
        produce(0);
        return;
    }

    char* endptr;
    long num = strtol(token.c_str(), &endptr, 0);
    if(endptr && *endptr) error("invalid number");
    produce((num >> 8) & 0xFF);
    produce(num & 0xFF);
}

static void for_code()
{
    while(!feof(fin)) {
        std::string token = getToken();
        switch(token[0]) {
        case '.':
            switch_mode(token);
            break;
        case ':':
            add_label_to_definitions(token);
            continue;
        case 'A':
            switch(token[1]) {
            case 'D':
                END(token, 2);
                produce(0xA);
                continue;
            case 'N':
                END(token, 2);
                produce(0x10);
                continue;
            default: error("invalid token");
            }
        case 'C':
            switch(token[1]) {
            case 'S':
                END(token, 2);
                produce(0x18);
                continue;
            case 'U':
                END(token, 2);
                produce(0x19);
                continue;
            default: error("invalid token");
            }
        case 'D':
            switch(token[1]) {
            case 'V':
                END(token, 2);
                produce(0xE);
                continue;
            default: error("invalid token");
            }
        case 'I':
            switch(token[1]) {
            case 'N':
                END(token, 2);
                produce(0x1);
                continue;
            default: error("invalid token");
            }
        case 'H':
            switch(token[1]) {
            case 'L':
                END(token, 2);
                produce(0x4);
                continue;
            default: error("invalid token");
            }
        case 'J':
            switch(token[1]) {
            case 'P':
                END(token, 2);
                produce(0x1E);
                continue;
            case 'Z':
                END(token, 2);
                produce(0x1F);
                continue;
            default: error("invalid token");
            }
        case 'L':
            switch(token[1]) {
            case 'D':
                END(token, 2);
                produce(0x8);
                continue;
            default: error("invalid token");
            }
        case 'M':
            switch(token[1]) {
            case 'O':
                END(token, 2);
                produce(0xD);
                continue;
            case 'U':
                END(token, 2);
                produce(0xC);
                continue;
            default: error("invalid token");
            }
        case 'N':
            switch(token[1]) {
            case 'E':
                END(token, 2);
                produce(0x17);
                continue;
            case 'O':
                END(token, 2);
                produce(0);
                continue;
            case 'T':
                END(token, 2);
                produce(0x13);
                continue;
            default: error("invalid token");
            }
        case 'O':
            switch(token[1]) {
            case 'R':
                END(token, 2);
                produce(0x11);
                continue;
            default: error("invalid token");
            }
        case 'P':
            switch(token[1]) {
            case 'I':
                END(token, 2);
                push_imed();
                continue;
            case 'P':
                END(token, 2);
                produce(0x3);
                continue;
            case 'R':
                if(token[2] != '.') error("expected register number");
                produce_reg(0x5, token.c_str());
                continue;
            default: error("invalid token");
            }
        case 'R':
            switch(token[1]) {
            case 'D':
                if(token[2] != '.') error("expected register number");
                produce_reg(0x7, token.c_str());
                continue;
            case 'I':
                if(token[2] != '.') error("expected register number");
                produce_reg(0x6, token.c_str());
                continue;
            case 'L':
                if(token[2] != '.') error("expected register number");
                produce_reg(0x2, token.c_str());
                continue;
            case 'M':
                if(token[2] != '.') error("expected register number");
                produce_reg(0x1, token.c_str());
                continue;
            case 'P':
                if(token[2] != '.') error("expected register number");
                produce_reg(0x4, token.c_str());
                continue;
            case 'R':
                if(token[2] != '.') error("expected register number");
                produce_reg(0x3, token.c_str());
                continue;
            case 'S':
                END(token, 2);
                produce(0x2);
                continue;
            case 'T':
                END(token, 2);
                produce(0x7);
                continue;
            default: error("invalid token");
            }
        case 'S':
            switch(token[1]) {
            case 'T':
                produce(0x9);
                continue;
            case 'U':
                produce(0xB);
                continue;
            case 'V':
                produce(0x6);
                continue;
            default: error("invalid token");
            }
        case 'X':
            switch(token[1]) {
            case 'R':
                END(token, 2);
                produce(0x12);
                continue;
            default: error("invalid token");
            }
        }
    }
}

static void assemble()
{
    mode = DATA;
    current_size = &data_pos;
    while(!feof(fin)) {
        asmmode_t prevMode = mode;
        switch(mode) {
        case DATA:
            for_data();
            break;
        case CODE:
            for_code();
            break;
        default: error("invalid section");
        }
        if(mode != prevMode) {
            switch(prevMode) {
            case DATA:
                data_pos = ftell(fout);
                fseek(fout, code_pos, SEEK_SET);
                current_size = &code_pos;
                break;
            case CODE:
                code_pos = ftell(fout);
                fseek(fout, data_pos, SEEK_SET);
                current_size = &data_pos;
                break;
            }
        }
    }

    resolve_labels();
}

//=============================================================
// main
//=============================================================

int main(int argc, char* argv[])
{
    if(argc != 2) throw 1;
    fin = fopen(argv[1], "r");
    std::string name = argv[1];
    auto p = name.rfind(".");
    if(p != std::string::npos) {
        name = name.substr(0, p) + ".hss";
        if(name.compare(argv[1]) == 0) name += ".out";
    } else {
        name += ".hss";
    }
    fout = fopen(name.c_str(), "w");
    ftruncate(fileno(fout), /*code*/0x10000 + /*data*/0x20000);
    clearOutputFile(fout, 0x10000 + 0x20000);

    code_pos = 0;
    data_pos = 0x10000;
    fseek(fout, data_pos, SEEK_SET);

    g_flags &= ~LOG_TOKENIZER & ~LOG_DATAGEN;

    try {
        assemble();
    } catch(...) {
        log(LOG_ERR, "Errors happened, compilation aborted\n");
        fclose(fin);
        fclose(fout);
        exit(255);
    }

    log(0, "DONE\n");
    fflush(stdout);

    fclose(fin);
    fclose(fout);

    return 0;
}
