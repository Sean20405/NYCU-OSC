#include "exception.h"

void exception_entry() {
    // Print spsr_el1, elr_el1, and esr_el1
    unsigned long spsr_el1, elr_el1, esr_el1;
    asm volatile(
        "mrs %0, spsr_el1\n"
        "mrs %1, elr_el1\n"
        "mrs %2, esr_el1\n"
        : "=r"(spsr_el1), "=r"(elr_el1), "=r"(esr_el1)
        :
        :
    );

    uart_puts("spsr_el1: ");
    uart_hex(spsr_el1);
    uart_puts(" elr_el1: ");
    uart_hex(elr_el1);
    uart_puts(" esr_el1: ");
    uart_hex(esr_el1);
    uart_puts("\r\n");
}