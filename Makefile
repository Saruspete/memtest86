# Makefile for MemTest86+
#
# Author:		Chris Brady
# Created:		January 1, 1996

#
# Path for the floppy disk device
#
FDISK=/dev/fd0

CC=gcc

CFLAGS=-Wall -march=i486 -m32 -Os -fomit-frame-pointer -fno-builtin -ffreestanding -fPIC

OBJS= head.o reloc.o main.o test.o init.o lib.o patn.o screen_buffer.o \
      config.o linuxbios.o memsize.o pci.o controller.o random.o extra.o \
      spd.o error.o dmi.o

all: memtest.bin memtest

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

memtest.bin: memtest_shared.bin bootsect.o setup.o memtest.bin.lds
	$(LD) -T memtest.bin.lds bootsect.o setup.o -b binary \
	memtest_shared.bin -o memtest.bin

reloc.o: reloc.c
	$(CC) -c $(CFLAGS) -fno-strict-aliasing reloc.c

test.o: test.c
	$(CC) -c -Wall -march=i486 -m32 -Os -fomit-frame-pointer -fno-builtin -ffreestanding test.c

clean:
	rm -f *.o memtest.bin memtest memtest_shared memtest_shared.bin

install: all
	dd <memtest.bin >$(FDISK) bs=8192

install-precomp:
	dd <precomp.bin >$(FDISK) bs=8192
