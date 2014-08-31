all: asm.bin jakvmhs.bin

.PHONY: all

asm.bin: asm.cpp
	g++ --std=gnu++11 -g -o asm.bin asm.cpp

jakvmhs.bin: jakvmhs.c jakvmhs.h sn.o sn.h
	gcc --std=gnu99 -g -o jakvmhs.bin jakvmhs.c sn.o -ldl -lstdc++

sn.o: sn.cpp sn.h
	g++ --std=gnu++11 -g -c sn.cpp

sn_test.bin: testsn.cpp sn.cpp sn.h
	g++ --std=gnu++11 -g testsn.cpp -o sn_test.bin

clean:
	rm -f *.bin
