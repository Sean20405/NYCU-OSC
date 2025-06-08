#ifndef MMU_H
#define MMU_H

#define PHYS_TO_VIRT(x) ((x) + 0xffff000000000000)
#define VIRT_TO_PHYS(x) ((x) - 0xffff000000000000)

#ifndef __ASSEMBLER__
extern void set_pgd(unsigned long pgd);
extern unsigned long get_pgd();
#endif

void map_pages(unsigned long *pgd, unsigned long va, unsigned long pa, unsigned long size, unsigned long flags);


#endif /* MMU_H */