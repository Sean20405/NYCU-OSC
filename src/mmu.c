#include "mmu_reg.h"
#include "mmu.h"
#include "mm.h"
#include "sched.h"

void three_level_translation_init(){
    unsigned long *pmd_1 = (unsigned long *) 0x3000;
    for(unsigned long i=0; i<512; i++){
        unsigned long base = 0x200000L * i; // 2 * 16^5 -> 2MB
        if(base >= DEVICE_BASE){ 
            //map as device
            pmd_1[i] = base | PD_ACCESS | PD_BLOCK | (MAIR_IDX_DEVICE_nGnRnE << 2) | PD_USER_ACCESS;
        }
        else{
            //map as normal
            pmd_1[i] = base | PD_ACCESS | PD_BLOCK | (MAIR_IDX_NORMAL_NOCACHE << 2);
        }
    }

    unsigned long *pmd_2 = (unsigned long *) 0x4000;
    for(unsigned long i=0; i<512; i++){
        unsigned long base = 0x40000000L + 0x200000L * i;
        pmd_2[i] = base | PD_ACCESS | PD_BLOCK | (MAIR_IDX_NORMAL_NOCACHE << 2);
    }

    unsigned long * pud = (unsigned long *) 0x2000;
    *pud =  PD_TABLE | (unsigned long) pmd_1;
    pud[1] =  PD_TABLE | (unsigned long) pmd_2;
    // *pud = PD_ACCESS + (MAIR_IDX_NORMAL_NOCACHE << 2) + PD_TABLE + (unsigned long) pmd_1;
    // pud[1] = PD_ACCESS + (MAIR_IDX_NORMAL_NOCACHE << 2) + PD_TABLE + (unsigned long) pmd_2;
}

void map_page(unsigned long *pgd, unsigned long va, unsigned long pa, unsigned long flags) {
    // uart_puts("Mapping page: VA = ");
    // uart_hex_long(va);
    // uart_puts(", PA = ");
    // uart_hex_long(pa);
    // uart_puts("\r\n");
    
    int pgd_idx, pud_idx, pmd_idx, pte_idx;
    unsigned long *pud, *pmd, *pte;

    pgd_idx = (va >> PGD_SHIFT) & (PTRS_PER_TABLE - 1);
    pud_idx = (va >> PUD_SHIFT) & (PTRS_PER_TABLE - 1);
    pmd_idx = (va >> PMD_SHIFT) & (PTRS_PER_TABLE - 1);
    pte_idx = (va >> PTE_SHIFT) & (PTRS_PER_TABLE - 1);

    // pgd = (unsigned long *)PHYS_TO_VIRT((unsigned long)pgd);
    // uart_puts("PGD @ ");
    // uart_hex_long((unsigned long)pgd);
    // uart_puts("\r\n");

    unsigned long *pgd_virt = (unsigned long *)PHYS_TO_VIRT((unsigned long)pgd);

    if (pgd_virt[pgd_idx] == 0) {
        // uart_puts("Allocating new PUD page for index ");
        // uart_puts(itoa(pgd_idx));
        // uart_puts(" @ ");

        pud = (unsigned long *)alloc(PAGE_SIZE);
        memset(pud, 0, PAGE_SIZE);

        // uart_hex_long((unsigned long)pud);
        // uart_puts("\r\n");

        pgd_virt[pgd_idx] = VIRT_TO_PHYS((unsigned long)pud) | PD_TABLE | PD_ACCESS| PD_USER_ACCESS | PD_RO;
    }
    pud = (unsigned long *)(PHYS_TO_VIRT((unsigned long)pgd_virt[pgd_idx] & PAGE_MASK));
    // uart_puts("PUD @ ");
    // uart_hex_long((unsigned long)pud);
    // uart_puts("\r\n");

    if (pud[pud_idx] == 0) {
        // uart_puts("Allocating new PMD page for index ");
        // uart_puts(itoa(pud_idx));
        // uart_puts(" @ ");

        pmd = (unsigned long *)alloc(PAGE_SIZE);
        memset(pmd, 0, PAGE_SIZE);

        // uart_hex_long((unsigned long)pmd);
        // uart_puts("\r\n");

        pud[pud_idx] = VIRT_TO_PHYS((unsigned long)pmd) | PD_TABLE | PD_ACCESS | PD_USER_ACCESS | PD_RO;
    }
    pmd = (unsigned long *)(PHYS_TO_VIRT((unsigned long)pud[pud_idx] & PAGE_MASK));
    // uart_puts("PMD @ ");
    // uart_hex_long((unsigned long)pmd);
    // uart_puts("\r\n");

    if (pmd[pmd_idx] == 0) {
        // uart_puts("Allocating new PTE page for index ");
        // uart_puts(itoa(pmd_idx));
        // uart_puts(" @ ");

        pte = (unsigned long *)alloc(PAGE_SIZE);
        memset(pte, 0, PAGE_SIZE);

        // uart_hex_long((unsigned long)pte);
        // uart_puts("\r\n");

        pmd[pmd_idx] = VIRT_TO_PHYS((unsigned long)pte) | PD_TABLE | PD_ACCESS | PD_USER_ACCESS | PD_RO;
    }
    pte = (unsigned long *)(PHYS_TO_VIRT((unsigned long)pmd[pmd_idx] & PAGE_MASK));
    // uart_puts("PTE @ ");
    // uart_hex_long((unsigned long)pte);
    // uart_puts("\r\n");

    pa = (pa & PAGE_MASK); // Ensure PA is page-aligned
    // uart_hex_long(pa);
    // uart_puts("\r\n");
    pte[pte_idx] = (pa) | flags | PD_ACCESS;
    // uart_hex_long(pte[pte_idx]);
    // uart_puts("\r\n");

    // uart_puts("Final PTE value for VA ");
    // uart_hex_long(va);
    // uart_puts(": ");
    // uart_hex_long(pte[pte_idx]);
    // uart_puts("\n");
}

void map_pages(unsigned long *pgd, unsigned long va, unsigned long pa, unsigned long size, unsigned long flags) {
    for (unsigned long i = 0; i < size; i += PAGE_SIZE) {
        map_page(pgd, va + i, pa + i, flags);
    }
}