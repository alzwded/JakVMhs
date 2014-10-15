all: asm.bin jakvmhs.bin

.PHONY: all

asm.bin: asm.cpp
	g++ --std=gnu++11 -g -o asm.bin asm.cpp

jakvmhs.bin: jakvmhs.c jakvmhs.h
	gcc --std=gnu99 -g -o jakvmhs.bin jakvmhs.c -ldl -lstdc++

libtestutils.so: jakvmhs.h testutils.c
	gcc -g -o libtestutils.so -shared -fPIC testutils.c

clean:
	rm -f *.bin *.so
