# 1 "head.S"
# 1 "<built-in>"
# 1 "<command line>"
# 1 "head.S"
# 14 "head.S"
.text

# 1 "defs.h" 1
# 17 "head.S" 2
# 1 "config.h" 1
# 18 "head.S" 2
# 1 "test.h" 1
# 19 "head.S" 2
# 35 "head.S"
 .code32
 .globl startup_32
startup_32:
 cld
 cli


 testl %esp, %esp
 jnz 0f
 movl $(0x00002000 + _GLOBAL_OFFSET_TABLE_), %esp
 leal stack_top@GOTOFF(%esp), %esp
0:


 call 0f
0: popl %ebx
 addl $_GLOBAL_OFFSET_TABLE_+[.-0b], %ebx


 leal stack_top@GOTOFF(%ebx), %esp


 leal gdt@GOTOFF(%ebx), %eax
 movl %eax, 2 + gdt_descr@GOTOFF(%ebx)
 lgdt gdt_descr@GOTOFF(%ebx)
 leal flush@GOTOFF(%ebx), %eax
 pushl $0x10
 pushl %eax
 lret
flush: movl $0x18, %eax
 movw %ax, %ds
 movw %ax, %es
 movw %ax, %fs
 movw %ax, %gs
 movw %ax, %ss




 cmpl $1, zerobss@GOTOFF(%ebx)
 jnz zerobss_done
 xorl %eax, %eax
 leal _bss@GOTOFF(%ebx), %edi
 leal _end@GOTOFF(%ebx), %ecx
 subl %edi, %ecx
1: movl %eax, (%edi)
 addl $4, %edi
 subl $4, %ecx
 jnz 1b
 movl $0, zerobss@GOTOFF(%ebx)
zerobss_done:




 cmpl $1, clear_display@GOTOFF(%ebx)
 jnz clear_display_done
 movw $0x0720, %ax
 movl $0xb8000, %edi
 movl $0xc0000, %ecx
1: movw %ax, (%edi)
 addl $2, %edi
 cmpl %ecx, %edi
 jnz 1b
 movl $0, clear_display@GOTOFF(%ebx)
clear_display_done:





 leal idt@GOTOFF(%ebx), %edi

 leal vec0@GOTOFF(%ebx), %edx
 movl $(0x10 << 16),%eax
 movw %dx, %ax
 movw $0x8E00, %dx
 movl %eax, (%edi)
 movl %edx, 4(%edi)
 addl $8, %edi

 leal vec1@GOTOFF(%ebx),%edx
 movl $(0x10 << 16),%eax
 movw %dx,%ax
 movw $0x8E00,%dx
 movl %eax,(%edi)
 movl %edx,4(%edi)
 addl $8,%edi

 leal vec2@GOTOFF(%ebx),%edx
 movl $(0x10 << 16),%eax
 movw %dx,%ax
 movw $0x8E00,%dx
 movl %eax,(%edi)
 movl %edx,4(%edi)
 addl $8,%edi

 leal vec3@GOTOFF(%ebx),%edx
 movl $(0x10 << 16),%eax
 movw %dx,%ax
 movw $0x8E00,%dx
 movl %eax,(%edi)
 movl %edx,4(%edi)
 addl $8,%edi

 leal vec4@GOTOFF(%ebx),%edx
 movl $(0x10 << 16),%eax
 movw %dx,%ax
 movw $0x8E00,%dx
 movl %eax,(%edi)
 movl %edx,4(%edi)
 addl $8,%edi

 leal vec5@GOTOFF(%ebx),%edx
 movl $(0x10 << 16),%eax
 movw %dx,%ax
 movw $0x8E00,%dx
 movl %eax,(%edi)
 movl %edx,4(%edi)
 addl $8,%edi

 leal vec6@GOTOFF(%ebx),%edx
 movl $(0x10 << 16),%eax
 movw %dx,%ax
 movw $0x8E00,%dx
 movl %eax,(%edi)
 movl %edx,4(%edi)
 addl $8,%edi

 leal vec7@GOTOFF(%ebx),%edx
 movl $(0x10 << 16),%eax
 movw %dx,%ax
 movw $0x8E00,%dx
 movl %eax,(%edi)
 movl %edx,4(%edi)
 addl $8,%edi

 leal vec8@GOTOFF(%ebx),%edx
 movl $(0x10 << 16),%eax
 movw %dx,%ax
 movw $0x8E00,%dx
 movl %eax,(%edi)
 movl %edx,4(%edi)
 addl $8,%edi

 leal vec9@GOTOFF(%ebx),%edx
 movl $(0x10 << 16),%eax
 movw %dx,%ax
 movw $0x8E00,%dx
 movl %eax,(%edi)
 movl %edx,4(%edi)
 addl $8,%edi

 leal vec10@GOTOFF(%ebx),%edx
 movl $(0x10 << 16),%eax
 movw %dx,%ax
 movw $0x8E00,%dx
 movl %eax,(%edi)
 movl %edx,4(%edi)
 addl $8,%edi

 leal vec11@GOTOFF(%ebx),%edx
 movl $(0x10 << 16),%eax
 movw %dx,%ax
 movw $0x8E00,%dx
 movl %eax,(%edi)
 movl %edx,4(%edi)
 addl $8,%edi

 leal vec12@GOTOFF(%ebx),%edx
 movl $(0x10 << 16),%eax
 movw %dx,%ax
 movw $0x8E00,%dx
 movl %eax,(%edi)
 movl %edx,4(%edi)
 addl $8,%edi

 leal vec13@GOTOFF(%ebx),%edx
 movl $(0x10 << 16),%eax
 movw %dx,%ax
 movw $0x8E00,%dx
 movl %eax,(%edi)
 movl %edx,4(%edi)
 addl $8,%edi

 leal vec14@GOTOFF(%ebx),%edx
 movl $(0x10 << 16),%eax
 movw %dx,%ax
 movw $0x8E00,%dx
 movl %eax,(%edi)
 movl %edx,4(%edi)
 addl $8,%edi

 leal vec15@GOTOFF(%ebx),%edx
 movl $(0x10 << 16),%eax
 movw %dx,%ax
 movw $0x8E00,%dx
 movl %eax,(%edi)
 movl %edx,4(%edi)
 addl $8,%edi

 leal vec16@GOTOFF(%ebx),%edx
 movl $(0x10 << 16),%eax
 movw %dx,%ax
 movw $0x8E00,%dx
 movl %eax,(%edi)
 movl %edx,4(%edi)
 addl $8,%edi

 leal vec17@GOTOFF(%ebx),%edx
 movl $(0x10 << 16),%eax
 movw %dx,%ax
 movw $0x8E00,%dx
 movl %eax,(%edi)
 movl %edx,4(%edi)
 addl $8,%edi

 leal vec18@GOTOFF(%ebx),%edx
 movl $(0x10 << 16),%eax
 movw %dx,%ax
 movw $0x8E00,%dx
 movl %eax,(%edi)
 movl %edx,4(%edi)
 addl $8,%edi

 leal vec19@GOTOFF(%ebx),%edx
 movl $(0x10 << 16),%eax
 movw %dx,%ax
 movw $0x8E00,%dx
 movl %eax,(%edi)
 movl %edx,4(%edi)
 addl $8,%edi


 leal idt@GOTOFF(%ebx), %eax
 movl %eax, 2 + idt_descr@GOTOFF(%ebx)
 lidt idt_descr@GOTOFF(%ebx)



 leal cpu_id@GOTOFF(%ebx), %esi
 movl %ebx, %edi

 movl $-1, 4(%esi) # -1 for no CPUID initially



 movl $3, 0(%esi) # at least 386
 pushfl # push EFLAGS
 popl %eax # get EFLAGS
 movl %eax, %ecx # save original EFLAGS
 xorl $0x40000, %eax # flip AC bit in EFLAGS
 pushl %eax # copy to EFLAGS
 popfl # set EFLAGS
 pushfl # get new EFLAGS
 popl %eax # put it in eax
 xorl %ecx, %eax # change in flags
 andl $0x40000, %eax # check if AC bit changed
 je id_done

 movl $4, 0(%esi) # at least 486
 movl %ecx, %eax
 xorl $0x200000, %eax # check ID flag
 pushl %eax
 popfl # if we are on a straight 486DX, SX, or
 pushfl # 487SX we can't change it
 popl %eax
 xorl %ecx, %eax
 pushl %ecx # restore original EFLAGS
 popfl
 andl $0x200000, %eax
 jne have_cpuid


 xorw %ax, %ax # clear ax
 sahf # clear flags
 movw $5, %ax
 movw $2, %bx
 div %bl # do operation that does not change flags
 lahf # get flags
 cmp $2, %ah # check for change in flags
 jne id_done # if not Cyrix
 movl $2, 0(%esi) # Use two to identify as Cyrix
 jmp id_done

have_cpuid:

 xorl %eax, %eax # call CPUID with 0 -> return vendor ID
 cpuid
 movl %eax, 4(%esi) # save CPUID level
 movl %ebx, 12(%esi) # first 4 chars
 movl %edx, 12 +4(%esi) # next 4 chars
 movl %ecx, 12 +8(%esi) # last 4 chars

 orl %eax, %eax # do we have processor info as well?
 je id_done

 movl $1, %eax # Use the CPUID instruction to get CPU type
 cpuid



 # CDH start
 # Check FPU, initialize if present

 testl $1, %edx # FPU available?
 jz no_fpu
 finit

 no_fpu:

 # CDH end


 movl %eax, 44(%esi) # save complete extended CPUID to 44
 movl %ecx, 48(%esi) # save ECX Feature Flags to 48
 movb %al, %cl # save reg for future use
 andb $0x0f, %ah # mask processor family
 movb %ah, 0(%esi)
 andb $0xf0, %al # mask model
 shrb $4, %al
 movb %al, 1(%esi)
 andb $0x0f, %cl # mask mask revision
 movb %cl, 2(%esi)
 movl %edx, 8(%esi)

 movl $0, 24(%esi)
 movl $0, 24 +4(%esi)
 movl $0, 24 +8(%esi)
 movl $0, 24 +12(%esi)

 movl 12 +8(%esi), %eax
 cmpl $0x6c65746e,%eax # Is this an Intel CPU? "GenuineIntel"
 jne not_intel
 movb %bl, 40(%esi) # Store BrandID in AMD PWRCAP if the CPU is from Intel
 movl $2, %eax # Use the CPUID instruction to get cache info
 cpuid
 movl %eax, 24(%esi)
 movl %ebx, 24 +4(%esi)
 movl %ecx, 24 +8(%esi)
 movl %edx, 24 +12(%esi)
 jmp id_done

not_intel:
 movl 12 +8(%esi),%eax
 cmpl $0x444d4163, %eax # Is this an AMD CPU? "AuthenticAMD"
 jne not_amd

 movl $0x80000005, %eax # Use the CPUID instruction to get cache info
 cpuid
 movl %ecx, 24(%esi)
 movl %edx, 24 +4(%esi)
 movl $0x80000006,%eax # Use the CPUID instruction to get cache info
 cpuid
 movl %ecx,24 +8(%esi)
 movl $0x80000007,%eax # Use the CPUID instruction to get AMD Powercap
 cpuid
 movl %edx,40(%esi)

not_amd:
 movl 12 +8(%esi), %eax
 cmpl $0x3638784D, %eax # Is this a Transmeta CPU? "GenuineTMx86"
 jne not_transmeta

 movl $0x80000000, %eax # Use the CPUID instruction to check for cache info
 cpuid
 cmp $6, %al # Is cache info available?
 jb id_done

 movl $0x80000005, %eax # Use the CPUID instruction to get L1 cache info
 cpuid
 movl %ecx, 24(%esi)
 movl %edx, 24 +4(%esi)
 movl $0x80000006, %eax # Use the CPUID instruction to get L2 cache info
 cpuid
 movl %ecx, 24 +8(%esi)

not_transmeta:
 movl 12 +8(%esi), %eax
 cmpl $0x64616574, %eax # Is this a Via/Cyrix CPU? "CyrixInstead"
 jne not_cyrix

 movl 4(%esi), %eax # get CPUID level
 cmpl $2, %eax # Is there cache information available ?
 jne id_done

 movl $2, %eax # Use the CPUID instruction to get cache info
 cpuid
 movl %edx, 24(%esi)

not_cyrix:
 movl 12 +8(%esi), %eax
 cmpl $0x736C7561, %eax # Is this a Via/Centaur CPU "CentaurHauls"
 jne not_centaur

 movl $0x80000000, %eax # Use the CPUID instruction to check for cache info
 cpuid
 cmp $6, %al # Is cache info available?
 jb id_done

 movl $0x80000005, %eax # Use the CPUID instruction to get L1 cache info
 cpuid
 movl %ecx, 24(%esi)
 movl %edx, 24 +4(%esi)
 movl $0x80000006, %eax # Use the CPUID instruction to get L2 cache info
 cpuid
 movl %ecx, 24 +8(%esi)


not_centaur:
id_done:
 movl %edi, %ebx

 leal _dl_start@GOTOFF(%ebx), %eax
 call *%eax
 call do_test

 pushfl
 pushl %cs
 call 0f
0: pushl $0
 pushl $257
 jmp int_hand

vec0:
 pushl $0
 pushl $0
 jmp int_hand
vec1:
 pushl $0
 pushl $1
 jmp int_hand

vec2:
 pushl $0
 pushl $2
 jmp int_hand

vec3:
 pushl $0
 pushl $3
 jmp int_hand

vec4:
 pushl $0
 pushl $4
 jmp int_hand

vec5:
 pushl $0
 pushl $5
 jmp int_hand

vec6:
 pushl $0
 pushl $6
 jmp int_hand

vec7:
 pushl $0
 pushl $7
 jmp int_hand

vec8:

 pushl $8
 jmp int_hand

vec9:
 pushl $0
 pushl $9
 jmp int_hand

vec10:

 pushl $10
 jmp int_hand

vec11:

 pushl $11
 jmp int_hand

vec12:

 pushl $12
 jmp int_hand

vec13:

 pushl $13
 jmp int_hand

vec14:

 pushl $14
 jmp int_hand

vec15:
 pushl $0
 pushl $15
 jmp int_hand

vec16:
 pushl $0
 pushl $16
 jmp int_hand

vec17:

 pushl $17
 jmp int_hand

vec18:
 pushl $0
 pushl $18
 jmp int_hand

vec19:
 pushl $0
 pushl $19
 jmp int_hand

int_hand:
 pushl %eax
 pushl %ebx
 pushl %ecx
 pushl %edx
 pushl %edi
 pushl %esi
 pushl %ebp


 leal 20(%esp), %eax
 pushl %eax

 pushl %esp
 call inter
 addl $8, %esp

 popl %ebp
 popl %esi
 popl %edi
 popl %edx
 popl %ecx
 popl %ebx
 popl %eax
 iret




.align 4
.word 0
idt_descr:
 .word 20*8-1 # idt contains 32 entries
 .long 0

idt:
 .fill 20,8,0 # idt is uninitialized

gdt_descr:
 .word gdt_end - gdt - 1
 .long 0

.align 4
.globl gdt, gdt_end
gdt:
 .quad 0x0000000000000000
 .quad 0x0000000000000000
 .quad 0x00cf9a000000ffff
 .quad 0x00cf92000000ffff

 .word 0xFFFF # 16bit 64KB - (0x10000*1 = 64KB)
 .word 0 # base address = (0x9000 +0x20)
 .byte 0x00, 0x9b # code read/exec/accessed
 .byte 0x00, 0x00 # granularity = bytes


 .word 0xFFFF # 16bit 64KB - (0x10000*1 = 64KB)
 .word 0 # base address = (0x9000 +0x20)
 .byte 0x00, 0x93 # data read/write/accessed
 .byte 0x00, 0x00 # granularity = bytes

gdt_end:

.data

.macro ptes64 start, count=64
.quad \start + 0x0000000 + 0xE3
.quad \start + 0x0200000 + 0xE3
.quad \start + 0x0400000 + 0xE3
.quad \start + 0x0600000 + 0xE3
.quad \start + 0x0800000 + 0xE3
.quad \start + 0x0A00000 + 0xE3
.quad \start + 0x0C00000 + 0xE3
.quad \start + 0x0E00000 + 0xE3
.if \count-1
ptes64 "(\start+0x01000000)",\count-1
.endif
.endm

.macro maxdepth depth=1
.if \depth-1
maxdepth \depth-1
.endif
.endm

maxdepth

.balign 4096
.globl pd0
pd0:
 ptes64 0x0000000000000000

.balign 4096
.globl pd1
pd1:
 ptes64 0x0000000040000000

.balign 4096
.globl pd2
pd2:
 ptes64 0x0000000080000000

.balign 4096
.globl pd3
pd3:
 ptes64 0x00000000C0000000

.balign 4096
.globl pdp
pdp:
 .long pd0 + 1
 .long 0
 .long pd1 + 1
 .long 0

 .long pd2 + 1
 .long 0

 .long pd3 + 1
 .long 0
.previous



 .globl query_pcbios
query_pcbios:

 pushl %ebx
 pushl %esi
 pushl %edi
 pushl %ebp
 call 1f
1: popl %ebx
 addl $_GLOBAL_OFFSET_TABLE_+[.-1b], %ebx


 leal startup_32@GOTOFF(%ebx), %esi


 movl %esi, %eax
 shrl $4, %eax
 movw %ax, 2 + realptr@GOTOFF(%ebx)


 leal prot@GOTOFF(%ebx), %eax
 movl %eax, protptr@GOTOFF(%ebx)


 movl %esi, %eax
 shll $16, %eax # Base low

 movl %esi, %ecx
 shrl $16, %ecx
 andl $0xff, %ecx

 movl %esi, %edx
 andl $0xff000000, %edx
 orl %edx, %ecx


 andl $0x0000ffff, 0x20 + 0 + gdt@GOTOFF(%ebx)
 orl %eax, 0x20 + 0 + gdt@GOTOFF(%ebx)
 andl $0x00ffff00, 0x20 + 4 + gdt@GOTOFF(%ebx)
 orl %ecx, 0x20 + 4 + gdt@GOTOFF(%ebx)
 andl $0x0000ffff, 0x28 + 0 + gdt@GOTOFF(%ebx)
 orl %eax, 0x28 + 0 + gdt@GOTOFF(%ebx)
 andl $0x00ffff00, 0x28 + 4 + gdt@GOTOFF(%ebx)
 orl %ecx, 0x28 + 4 + gdt@GOTOFF(%ebx)


 leal gdt@GOTOFF(%ebx), %eax
 movl %eax, 2 + gdt_descr@GOTOFF(%ebx)

 lidt idt_real@GOTOFF(%ebx)




 movl $0x28, %eax
 movl %eax, %ds
 movl %eax, %es
 movl %eax, %ss
 movl %eax, %fs
 movl %eax, %gs


 leal stack@GOTOFF(%ebx), %ecx

 leal mem_info@GOTOFF(%ebx), %edi


 ljmp $0x20, $1f - startup_32
1:
 .code16


 movl %cr0,%eax
 andl $~((1 << 31)|(1<<0)),%eax
 movl %eax,%cr0




 ljmp *(realptr - startup_32)
real:



 movw %cs, %ax
 movw %ax, %ds
 movw %ax, %es
 movw %ax, %fs
 movw %ax, %gs
 movw %ax, %ss


 movl %ecx, %eax
 shrl $4, %eax
 movw %ax, %ss
 subl %ecx, %esp


 pushl %ebx


 shrl $4, %edi
 movl %edi, %ds


 sti

# Get memory size (extended mem, kB)



 xorl %eax, %eax
 movl %eax, (0x00)
 movl %eax, (0x04)
 movl %eax, (0x08)

# Try three different memory detection schemes. First, try
# e820h, which lets us assemble a memory map, then try e801h,
# which returns a 32-bit memory size, and finally 88h, which
# returns 0-64m

# method E820H:
# the memory map from hell. e820h returns memory classified into
# a whole bunch of different types, and allows memory holes and
# everything. We scan through this memory map and build a list
# of the first 32 memory areas, which we return at [0x0c].
# This is documented at http:

meme820:
 xorl %ebx, %ebx # continuation counter
 movw $0x0c, %di # point into the whitelist
      # so we can have the bios
      # directly write into it.

jmpe820:
 movl $0x0000e820, %eax # e820, upper word zeroed
 movl $0x534d4150, %edx # ascii 'SMAP'
 movl $20, %ecx # size of the e820rec
 pushw %ds # data record.
 popw %es
 int $0x15 # make the call
 jc bail820 # fall to e801 if it fails

 cmpl $0x534d4150, %eax # check the return is `0x534d4150'
 jne bail820 # fall to e801 if it fails

# cmpl $1, 16(%di) # is this usable memory?
# jne again820

 # If this is usable memory, we save it by simply advancing %di by
 # sizeof(e820rec).

good820:
 movb (0x08), %al # up to 32 entries
 cmpb $32, %al
 jnl bail820

 incb (0x08)
 movw %di, %ax
 addw $20, %ax
 movw %ax, %di
again820:
 cmpl $0, %ebx # check to see if
 jne jmpe820 # %ebx is set to EOF
bail820:


# method E801H:
# memory size is in 1k chunksizes, to avoid confusing loadlin.
# we store the 0xe801 memory size in a completely different place,
# because it will most likely be longer than 16 bits.

meme801:
 stc # fix to work around buggy
 xorw %cx,%cx # BIOSes which dont clear/set
 xorw %dx,%dx # carry on pass/error of
      # e801h memory size call
      # or merely pass cx,dx though
      # without changing them.
 movw $0xe801, %ax
 int $0x15
 jc mem88

 cmpw $0x0, %cx # Kludge to handle BIOSes
 jne e801usecxdx # which report their extended
 cmpw $0x0, %dx # memory in AX/BX rather than
 jne e801usecxdx # CX/DX. The spec I have read
 movw %ax, %cx # seems to indicate AX/BX
 movw %bx, %dx # are more reasonable anyway...

e801usecxdx:
 andl $0xffff, %edx # clear sign extend
 shll $6, %edx # and go from 64k to 1k chunks
 movl %edx, (0x04) # store extended memory size
 andl $0xffff, %ecx # clear sign extend
  addl %ecx, (0x04) # and add lower memory into
      # total size.

# Ye Olde Traditional Methode. Returns the memory size (up to 16mb or
# 64mb, depending on the bios) in ax.
mem88:

 movb $0x88, %ah
 int $0x15
 movw %ax, (0x00)


# check for APM BIOS
 movw $0x5300, %ax # APM BIOS installation check
 xorw %bx, %bx
 int $0x15
 jc done_apm_bios # error -> no APM BIOS

 cmpw $0x504d, %bx # check for "PM" signature
 jne done_apm_bios # no signature -> no APM BIOS

 movw $0x5304, %ax # Disconnect first just in case
 xorw %bx, %bx
 int $0x15 # ignore return code

 movw $0x5301, %ax # Real Mode connect
 xorw %bx, %bx
 int $0x15
 jc done_apm_bios # error

 movw $0x5308, %ax # Disable APM
 mov $0xffff, %bx
 xorw %cx, %cx
 int $0x15

done_apm_bios:



 cli


 popl %ebx


 movw %cs, %ax
 movw %ax, %ds


 addr32 lgdt gdt_descr - startup_32



 movl %cr0,%eax
 orl $(1<<0),%eax
 movl %eax,%cr0


 data32 ljmp *(protptr - startup_32)
prot:
 .code32

 movl $0x18, %eax
 movl %eax, %ds
 movl %eax, %es
 movl %eax, %fs
 movl %eax, %gs
 movl %eax, %ss


 leal stack@GOTOFF(%ebx), %eax
 addl %eax, %esp


 popl %ebp
 popl %edi
 popl %esi
 popl %ebx
 movl $1, %eax
 ret

realptr:
 .word real - startup_32
 .word 0x0000
protptr:
 .long 0
 .long 0x10

idt_real:
 .word 0x400 - 1 # idt limit ( 256 entries)
 .word 0, 0 # idt base = 0L

.data
zerobss: .long 1
clear_display: .long 1
.previous
.data
.balign 16
 .globl mem_info
mem_info:
 . = . + 0x28c
.previous
.bss
.balign 16
stack:
 . = . + 4096
stack_top:
.previous
