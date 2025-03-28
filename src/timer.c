#include "timer.h"

void core_timer_entry() {
    // Print the seconds after booting
    unsigned long long cntpct_el0 = 0;
    unsigned long long cntfrq_el0 = 0;
    asm volatile("mrs %0, cntpct_el0" : "=r"(cntpct_el0));
    asm volatile("mrs %0, cntfrq_el0" : "=r"(cntfrq_el0));
    unsigned long long elapsed = cntpct_el0 / cntfrq_el0;

    uart_puts("Core timer interrupt, elapsed: ");
    uart_hex(elapsed);
    uart_puts("\r\n");

    // Set the next timeout to 2 seconds later
    asm volatile("msr cntp_tval_el0, %0" : : "r"(2 * cntfrq_el0));
}