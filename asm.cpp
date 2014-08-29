#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <string>
#include <map>
#include <algorithm>
#include <cstdio>
#include <sys/types.h>
#include <cctype>

enum asmmode_t {
    ERROR = 0,
    DATA,
    CODE
} mode = ERROR;

FILE* fin = NULL,* fout = NULL;
size_t data_pos, code_pos;
size_t* current_size;

static void error()
{
    size_t pos = ftell(fin);
    fprintf(stderr, "Error @%ld\n", pos);
    fflush(stderr);
    throw 1;
}

//=============================================================
// internal
//=============================================================

#define cassert(X) (!(X) ? fprintf(stderr, "Assertion failed: %s @%ld\n", #X, ftell(fin)), fflush(stderr), throw 0, 0 : 1)
#define END(X, N) if(X[N] != '\0') error();

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

    fprintf(stdout, "next token: '%s'\n", tok);
    return tok;
}

std::map<std::string, size_t> label_definitions;
std::map<size_t, std::string> label_usages;

static void add_label_to_definitions(std::string const& token)
{
    fprintf(stdout, "label %s defined at %lX\n", token.c_str(), *current_size);
    label_definitions.insert(std::make_pair(std::string(token), *current_size));
}

static void add_label_used_at(size_t size, std::string const& token)
{
    fprintf(stdout, "label %s used at %lX\n", token.c_str(), *current_size);
    label_usages.insert(std::make_pair(size, std::string(token)));
}

static void resolve_labels()
{
    std::for_each(label_usages.begin(), label_usages.end(), [&](decltype(label_usages)::value_type const& lbl){
        auto found = label_definitions.find(lbl.second);
        if(found == label_definitions.end()) error();
        unsigned short data = ((found->second >> 8) & 0xFF) | ((found->second & 0xFF) << 8);

        fseek(fout, lbl.first, SEEK_SET);
        fwrite(&data, sizeof(unsigned short), 1, fout);

        fprintf(stdout, "resolving %s to %X\n", lbl.second.c_str(), (int)data);
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
            default: error();
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
            default: error();
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
            default: error();
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
        default: error();
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
        long size = atol(ssize.c_str());

        fprintf(stdout, "need to consume: %ld\n", size);

        while(size > 0 && !feof(fin)) {
            std::string name = getToken();
            if(name[0] == '\'') {
                size_t i = 1;
                auto pEnd = name.substr(1).rfind('\'');
                cassert(pEnd != std::string::npos);
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

                    short data = ((unsigned char)c1) | ((unsigned char)c2 << 8);
                    fwrite(&data, sizeof(short), 1, fout);
                    *current_size = ftell(fout);
                    size--;
                    i += 2;
                    fprintf(stdout, "one byte down after %02X%02X\n", (int)c1, (int)c2);
                }
                fprintf(stdout, "after %s still need %ld\n", name.c_str(), size);
            } else {
                long num = atol(name.c_str());
                short data = ((num >> 8) & 0xFF) | ((num & 0xFF) << 8);
                fwrite(&data, sizeof(short), 1, fout);
                *current_size = ftell(fout);
                size--;
                fprintf(stdout, "after %s still need %ld\n", name.c_str(), size);
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
    if(endptr && *endptr) error();
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
            default: error();
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
            default: error();
            }
        case 'D':
            switch(token[1]) {
            case 'V':
                END(token, 2);
                produce(0xE);
                continue;
            default: error();
            }
        case 'I':
            switch(token[1]) {
            case 'N':
                END(token, 2);
                produce(0x1);
                continue;
            default: error();
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
            default: error();
            }
        case 'L':
            switch(token[1]) {
            case 'D':
                END(token, 2);
                produce(0x8);
                continue;
            default: error();
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
            default: error();
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
            default: error();
            }
        case 'O':
            switch(token[1]) {
            case 'R':
                END(token, 2);
                produce(0x11);
                continue;
            default: error();
            }
        case 'P':
            switch(token[1]) {
            case 'I':
                END(token, 2);
                push_imed();
                continue;
            case 'R':
                if(token[2] != '.') error();
                produce_reg(0x5, token.c_str());
                continue;
            default: error();
            }
        case 'R':
            switch(token[1]) {
            case 'D':
                if(token[2] != '.') error();
                produce_reg(0x7, token.c_str());
                continue;
            case 'I':
                if(token[2] != '.') error();
                produce_reg(0x6, token.c_str());
                continue;
            case 'L':
                if(token[2] != '.') error();
                produce_reg(0x2, token.c_str());
                continue;
            case 'M':
                if(token[2] != '.') error();
                produce_reg(0x1, token.c_str());
                continue;
            case 'P':
                if(token[2] != '.') error();
                produce_reg(0x4, token.c_str());
                continue;
            case 'R':
                if(token[2] != '.') error();
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
            default: error();
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
            default: error();
            }
        case 'X':
            switch(token[1]) {
            case 'R':
                END(token, 2);
                produce(0x12);
                continue;
            default: error();
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
        default: error();
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

    assemble();

    fprintf(stdout, "DONE\n");
    fflush(stdout);

    fclose(fin);
    fclose(fout);

    return 0;
}
