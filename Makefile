# Makefile for MemTest86+
#
# Author:		Chris Brady
# Created:		January 1, 1996


#
# Path for the floppy disk device
#
FDISK=/dev/fd0

AS=as -32
CC=gcc

CFLAGS= -Wall -Werror -march=i486 -m32 -O1 -fomit-frame-pointer -fno-builtin \
 -ffreestanding -fPIC $(SMP_FL) -fno-stack-protector

SELF_TEST_CFLAGS = -Wall -Werror -march=i486 -m32 -O1 -g

OBJS= head.o reloc.o main.o test.o init.o lib.o patn.o screen_buffer.o \
      config.o cpuid.o linuxbios.o pci.o spd.o error.o dmi.o controller.o \
      smp.o vmem.o memsize.o random.o

SELF_TEST_OBJS = test.o self_test.o cpuid.o random.o

all: clean memtest.bin memtest

run_self_test : self_test
	./self_test && touch run_self_test

# Link it statically once so I know I don't have undefined
# symbols and then link it dynamically so I have full
# relocation information
memtest_shared: $(OBJS) memtest_shared.lds Makefile
	$(LD) --warn-constructors --warn-common -static -T memtest_shared.lds \
	 -o $@ $(OBJS) && \
	$(LD) -shared -Bsymbolic -T memtest_shared.lds -o $@ $(OBJS)

memtest_shared.bin: memtest_shared
	objcopy -O binary $< memtest_shared.bin

memtest: memtest_shared.bin memtest.lds
	$(LD) -s -T memtest.lds -b binary memtest_shared.bin -o $@

self_test : $(SELF_TEST_OBJS)
	$(CC) $(SELF_TEST_CFLAGS) -o $@ $(SELF_TEST_OBJS)

head.s: head.S config.h defs.h test.h
	$(CC) -E -traditional $< -o $@

bootsect.s: bootsect.S config.h defs.h
	$(CC) -E -traditional $< -o $@

setup.s: setup.S config.h defs.h
	$(CC) -E -traditional $< -o $@

memtest.bin: memtest_shared.bin bootsect.o setup.o memtest.bin.lds
	$(LD) -T memtest.bin.lds bootsect.o setup.o -b binary \
	memtest_shared.bin -o memtest.bin

self_test.o : self_test.c
	$(CC) -c $(SELF_TEST_CFLAGS) self_test.c

memsize.o: memsize.c
	$(CC) -Wall -Werror -march=i486 -m32 -O0 -fomit-frame-pointer -fno-builtin -ffreestanding -fPIC $(SMP_FL) -fno-stack-protector   -c -o memsize.o memsize.c

random.o: random.c
	$(CC) -c -Wall -march=i486 -m32 -O3 -fomit-frame-pointer -fno-builtin -ffreestanding random.c

clean:
	rm -f *.o *.s *.iso memtest.bin memtest memtest_shared \
		memtest_shared.bin memtest.iso run_self_test self_test

iso:
	make all
	./makeiso.sh

install: all
	dd <memtest.bin >$(FDISK) bs=8192

install-precomp:
	dd <precomp.bin >$(FDISK) bs=8192

dos: all
	cat mt86+_loader memtest.bin > memtest.exe

debug2pxe: all
	cp memtest.bin /srv/tftp/memtest/memtest
