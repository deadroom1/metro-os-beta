CC      = gcc
AS      = nasm
LD      = ld
GRUB    = grub-mkrescue

CFLAGS  = -m32 -ffreestanding -O2 -Wall -Wextra -fno-stack-protector \
           -nostdlib -nostdinc -fno-builtin -Ilib
LDFLAGS = -m elf_i386 -T linker.ld --nostdlib

SRCS_C  = kernel/kernel.c \
          drivers/vga.c \
          drivers/keyboard.c \
          drivers/serial.c \
          errors/rsod.c \
          commands/cmd_mem.c \
          commands/cmd_time.c \
          commands/cmd_copy.c \
          commands/sys_cmd.c

SRCS_ASM = boot/boot.asm

OBJ_C   = $(SRCS_C:.c=.o)
OBJ_ASM = $(SRCS_ASM:.asm=.o)
OBJS    = $(OBJ_ASM) $(OBJ_C)

ISO     = metro-os.iso
KERNEL  = metro.bin

.PHONY: all iso run clean

all: $(KERNEL)

$(KERNEL): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: %.asm
	$(AS) -f elf32 $< -o $@

iso: $(KERNEL)
	mkdir -p isodir/boot/grub
	cp $(KERNEL) isodir/boot/metro.bin
	cp grub.cfg isodir/boot/grub/grub.cfg
	$(GRUB) -o $(ISO) isodir

run: iso
	qemu-system-i386 -cdrom $(ISO) -m 128M

clean:
	rm -f $(OBJS) $(KERNEL) $(ISO)
	rm -rf isodir
