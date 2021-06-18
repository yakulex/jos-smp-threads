/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>

###################################################################
# entry point for APs
###################################################################

# Each non-boot CPU ("AP") is started up in response to a STARTUP
# IPI from the boot CPU.  Section B.4.2 of the Multi-Processor
# Specification says that the AP will start in real mode with CS:IP
# set to XY00:0000, where XY is an 8-bit value sent with the
# STARTUP. Thus this code must start at a 4096-byte boundary.
#
# Because this code sets DS to zero, it must run from an address in
# the low 2^16 bytes of physical memory.
#
# boot_aps() (in init.c) copies this code to MPENTRY_PADDR (which
# satisfies the above restrictions).  Then, for each AP, it stores the
# address of the pre-allocated per-core stack in mpentry_kstack, sends
# the STARTUP IPI, and waits for this code to acknowledge that it has
# started (which happens in mp_main in init.c).
#
# This code is similar to boot/boot.S except that
#    - it does not need to enable A20
#    - it uses MPBOOTPHYS to calculate absolute addresses of its
#      symbols, rather than relying on the linker to fill them

#define RELOC(x) ((x) - KERNBASE)
#define MPBOOTPHYS(s) ((s) - mpentry_start + MPENTRY_PADDR)

.set MP_LINEAR_CODE_SEL, 0x8    # kernel code segment selector
.set MP_LINEAR_DATA_SEL, 0x10   # kernel data segment selector
.set MP_LINEAR_CODE64_SEL, 0x18
.set MP_LINEAR_DATA64_SEL, 0x20

.code16           
.globl mpentry_start
mpentry_start:
    cli            

    xorw    %ax, %ax
    movw    %ax, %ds
    movw    %ax, %es
    movw    %ax, %ss

    lgdt    MPBOOTPHYS(gdtdesc)
    movl    %cr0, %eax
    orl     $CR0_PE, %eax
    movl    %eax, %cr0

    ljmpl   $(MP_LINEAR_CODE_SEL), $(MPBOOTPHYS(start32))

.code32
start32:
    # Disable interrupts.
    cli
    # 1. Disable paging.
    movl    %cr0, %ecx
    andl    $(~CR0_PG), %ecx
    movl    %ecx, %cr0
    # 2. Switch to our GDT that supports 64-bit mode and update CS to MP_LINEAR_CODE_SEL.
    
    lgdt MPBOOTPHYS(gdtdesc)
    ljmpl $(MP_LINEAR_CODE_SEL), $(MPBOOTPHYS(othergdt32))

othergdt32:
    
    # 3. Reset all the data segment registers to linear mode (MP_LINEAR_DATA_SEL).
    movl $MP_LINEAR_DATA_SEL, %eax
    movw %ax, %ds
    movw %ax, %es
    movw %ax, %fs
    movw %ax, %gs
    movw %ax, %ss

    # 4. Enable PAE/PGE in CR4, which is required to transition to long mode.
    # This may already be enabled by the firmware but is not guaranteed.
    movl    %cr4, %ecx
    orl     $(CR4_PAE|CR4_PGE), %ecx
    movl    %ecx, %cr4

    # 5. Set up initial page table. We cannot use kern_pgdir yet because
    # we are still running at a low EIP.
    movl    $pml4phys, %eax
    movl    %eax, %cr3

    # 6. Enable long mode (LME) and execute protection (NXE) via the EFER MSR register.
    movl    $EFER_MSR, %ecx
    
    # EDX:EAX <- MSR[ECX]
    
    rdmsr
    orl     $(EFER_LME|EFER_NXE), %eax
    wrmsr

    # 7. Enable paging as it is required in 64-bit mode.
    movl    %cr0, %eax
    orl     $(CR0_PE|CR0_PG|CR0_WP|CR0_AM), %eax
    movl    %eax, %cr0

    # 8. Transition to 64-bit mode by updating CS with MP_LINEAR_CODE64_SEL.
    ljmpl $(MP_LINEAR_CODE64_SEL), $(MPBOOTPHYS(start64))

.code64
start64:
    # 9. Reset all the data segment registers to linear 64-bit mode (MP_LINEAR_DATA64_SEL).

    movq $MP_LINEAR_DATA64_SEL, %rax
    movl %eax, %ds
    movl %eax, %es
    movl %eax, %fs
    movl %eax, %gs
    movl %eax, %ss
    
    movabs $mp64entry, %rax
    jmpq *%rax


# Bootstrap GDT
.p2align 2                  # force 4 byte alignment
gdt:
    SEG_NULL
    SEG(STA_X | STA_R, 0x0, 0xFFFFFFFF) # code seg               # null seg
    SEG(STA_W, 0x0, 0xFFFFFFFF) # data seg  
    SEG64(STA_X | STA_R, 0x0, 0xFFFFFFFF)
    SEG64(STA_W, 0x0, 0xFFFFFFFF)

gdtdesc:
    .word   0x28 - 1                # sizeof(gdt) - 1
    .long   MPBOOTPHYS(gdt)         # address gdt

.globl mpentry_end
mpentry_end:
    nop
