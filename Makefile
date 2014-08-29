asm.bin: asm.cpp
	g++ --std=gnu++11 -g -o asm.bin asm.cpp

jakvmhs.bin: jakvmhs.c
	gcc --std=c99 -g -o jakvmhs.bin jakvmhs.c

clean:
	rm -f *.bin