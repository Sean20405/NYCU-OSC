#ifndef MMU_REG_H
#define MMU_REG_H

#define KERNEL_VA_BASE          0xffff000000000000
#define DEVICE_BASE             0x3f000000

#define PAGE_SHIFT              12
#define TABLE_SHIFT 			9
#define SECTION_SHIFT			(PAGE_SHIFT + TABLE_SHIFT)
#define SECTION_SIZE			(1 << SECTION_SHIFT)
#define PGD_SHIFT				(PAGE_SHIFT + 3 * TABLE_SHIFT)
#define PUD_SHIFT				(PAGE_SHIFT + 2 * TABLE_SHIFT)
#define PMD_SHIFT				(PAGE_SHIFT + TABLE_SHIFT)
#define PTE_SHIFT				PAGE_SHIFT

#define PHYS_MEMORY_SIZE        0x3C000000
#define PTRS_PER_TABLE			(1 << TABLE_SHIFT)
#define PAGE_MASK               (~(PAGE_SIZE - 1))

#define TCR_CONFIG_REGION_48bit (((64 - 48) << 0) | ((64 - 48) << 16))
#define TCR_CONFIG_4KB          ((0b00 << 14) |  (0b10 << 30))
#define TCR_CONFIG_DEFAULT      (TCR_CONFIG_REGION_48bit | TCR_CONFIG_4KB)

#define MAIR_DEVICE_nGnRnE      0b00000000
#define MAIR_NORMAL_NOCACHE     0b01000100
#define MAIR_IDX_DEVICE_nGnRnE  0
#define MAIR_IDX_NORMAL_NOCACHE 1
#define MAIR_VALUE              (MAIR_DEVICE_nGnRnE << (MAIR_IDX_DEVICE_nGnRnE * 8) | MAIR_NORMAL_NOCACHE << (MAIR_IDX_NORMAL_NOCACHE * 8))

/* Page descriptor flags */
#define PD_TABLE        0b11
#define PD_BLOCK        0b01
#define PD_PAGE         0b11
#define PD_ACCESS       (1 << 10)   // The access flag, a page fault is generated if not set.
#define PD_USER_ACCESS  (1 << 6)    // 0 for only kernel access, 1 for user/kernel access.
#define PD_RO           (1 << 7)    // 0 for read-write, 1 for read-only.
#define PD_UXN          (1 << 54)   // unprivileged execute-never bit: 1 for non-executable in EL0
#define PD_PXN          (1 << 53)   // privileged execute-never bit: 1 for non-executable in EL1

/* Pre-defined attributes */
#define BOOT_PGD_ATTR           PD_TABLE
#define BOOT_PUD_ATTR           (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)
#define BOOT_DEVICE_ATTR        (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_BLOCK)
#define BOOT_NORMAL_ATTR        (PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_BLOCK | PD_USER_ACCESS)
#define PTE_DEVICE_ATTR  (PD_ACCESS | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_PAGE) // for deveice-memory
#define PTE_NORAL_ATTR   (PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_PAGE) // for normal-memory

#endif // MMU_REG_H