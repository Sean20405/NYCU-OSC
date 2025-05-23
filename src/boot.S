#include "mmu_reg.h"

.section ".text.boot"

.global _start
_start:  // read cpu id, stop slave cores
    mov     x20, x0        // x20: dtb_addr. WARNING: only used for debugging
    mrs     x0, mpidr_el1  // read multiprocessor id register
    and     x0, x0, #0xff  // get cpu id
    cbz     x0, master     // if cpu id is 0, branch to master
    b       proc_hang      // otherwise, hang the core

master:
    bl      from_el2_to_el1

init_vm:
    ldr     x0, =TCR_CONFIG_DEFAULT
    msr     tcr_el1, x0

    ldr     x0, =MAIR_VALUE
    msr     mair_el1, x0

create_page_tables:
    // Preserve 0x0 - 0x1000 for spin table
    mov     x0, 0x1000          // PGD's page frame at 0x1000
    mov     x1, 0x2000          // PUD's page frame at 0x2000

    ldr     x2, =BOOT_PGD_ATTR
    orr     x2, x1, x2          // combine the physical address of next level page with attribute.
    str     x2, [x0]

    ldr     x2, =BOOT_PGD_ATTR  // Or BOOT_PGD_ATTR
    mov     x3, 0x3000          // PMD's page frame at 0x3000
    orr     x3, x2, x3
    str     x3, [x1]            // 1st 1GB mapped by the 1st entry of PUD
    ldr     x2, =BOOT_DEVICE_ATTR
    mov     x3, 0x40000000      // Device memory at 0x40000000
    orr     x3, x2, x3
    str     x3, [x1, 8]         // 2nd 1GB mapped by the 2nd entry of PUD

    mov     x2, 0x00000000
    mov     x3, 0x40000000
    ldr     x4, =BOOT_NORMAL_ATTR
    ldr     x5, =BOOT_DEVICE_ATTR
    mov     x6, 512                 // 512 entries in a PMD (1GB/2MB = 512)
    mov     x7, 0x3000              // PMD's address

fine_grain_ram_mmio:
    cbz     x6, boot_mmu
    cmp     x2, x3
    bge     mmio_region

normal_ram:
    orr     x8, x2, x4
    str     x8, [x7], #8
    add     x2, x2, #0x200000   // 2MB
    sub     x6, x6, 1
    b       fine_grain_ram_mmio

mmio_region:
    orr     x8, x2, x5
    str     x8, [x7], #8
    add     x2, x2, #0x200000   // 2MB
    sub     x6, x6, 1
    b       fine_grain_ram_mmio

boot_mmu:
    msr     ttbr0_el1, x0       // load PGD to the bottom translation-based register -> used for user space
    msr     ttbr1_el1, x0       // upper -> kernel space

    mrs     x2, sctlr_el1
    orr     x2 , x2, 1
    msr     sctlr_el1, x2       // enable MMU, cache remains disabled

    ldr     x2, =boot_rest      // indirect branch to the virtual address
    br      x2

boot_rest:
    bl      set_exception_vector_table

    // Set the stack pointer
    ldr     x0, =_start
    mov     sp, x0

    // Clear the BSS section
	ldr	    x0, =__bss_begin
	ldr	    x1, =__bss_end
    sub     x1, x1, x0
    cbz     x1, main
    bl      memzero

    // Call the main function
    bl      main

memzero: 
    /** 
     * Set the memory to zero
     * x0: pointer to the memory region
     * x1: size of the memory region 
     */
	str     xzr, [x0], #8
	subs    x1, x1, #1
	b.ne    memzero
	ret

proc_hang:
    wfe
    b       proc_hang

from_el2_to_el1:
    /*
     * From ARMv8-A Reference Manual:
     *     HCR_ELx, Hypervisor Configuration Register: D8.2.32 (P1923)
     *         [31] : RW. Execution state control for EL1. 1 for aarch64
     *     SPSR_ELx, Saved Program Status Registers: D1.6.4 (P1417)
     *         [3:2]: Target exception level.
     *         [0]  : SPSel. Selects the stack pointer for the target Exception level.
     *         [8:6]: Interrupt mask bits. 1 for disable. A, I, F
     *         [9]  : Debug exception mask bit. 1 for disable
     *     ELR_ELx, Exception Link Register: D1.6.5 (P1420)
    */
    mov     x0, (1 << 31)   // EL1 uses aarch64
    msr     hcr_el2, x0

    mov     x0, 0x345       // EL1h (SPSel = 1) with interrupt disabled.
                            // 3c5 = ... 0011 1100 0101
                            // 345 = ... 0011 0100 0101 (enable IRQ)
    msr     spsr_el2, x0

    msr     elr_el2, lr
    eret                    // Exception Return. Return to EL1