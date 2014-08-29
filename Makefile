all: asm.bin jakvmhs.bin

.PHONY: all

asm.bin: asm.cpp
	g++ --std=gnu++11 -g -o asm.bin asm.cpp

jakvmhs.bin: jakvmhs.c
	gcc --std=c99 -g -o jakvmhs.bin jakvmhs.c -ldl

clean:
	rm -f *.bin
