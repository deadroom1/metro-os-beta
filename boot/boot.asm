bits 32

MULTIBOOT_HEADER_MAGIC     equ 0x1BADB002
MULTIBOOT_FLAGS            equ 0x00000003
MULTIBOOT_CHECKSUM         equ -(MULTIBOOT_HEADER_MAGIC + MULTIBOOT_FLAGS)

section .multiboot
align 4
    dd MULTIBOOT_HEADER_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:

section .text
global _start
extern kernel_main

_start:
    mov esp, stack_top
    push ebx        ; multiboot info pointer
    push eax        ; multiboot magic
    call kernel_main
    cli
.hang:
    hlt
    jmp .hang
