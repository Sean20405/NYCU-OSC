#include "uart.h"
#include "shell.h"
#include "devicetree.h"
#include "timer.h"
#include "mm.h"
#include "sched.h"
#include "utils.h"
#include "syscall.h"
#include "exec.h"

extern char *kernel_start;
extern char *__stack_top;
extern uint64_t cpio_addr;
extern uint64_t cpio_end;
extern uint32_t fdt_total_size;

void foo(){
    for(int i = 0; i < 10; ++i) {
        unsigned int thread_id = ((struct ThreadTask *)get_current())->id;
        uart_puts("Thread id: ");
        uart_puts(itoa(thread_id));
        uart_puts(" ");
        uart_puts(itoa(i));
        uart_puts("\n");
        
        delay(1000000);
        schedule();
    }
    _exit();
}

void fork_test(){
    uart_puts("\r\nFork Test, pid ");
    uart_puts(itoa(get_pid()));
    uart_puts("\r\n");

    int cnt = 1;
    int ret = 0;
    if ((ret = fork()) == 0) { // child
        long long cur_sp;
        asm volatile("mov %0, sp" : "=r"(cur_sp));

        uart_puts("first child pid: ");
        uart_puts(itoa(get_pid()));
        uart_puts(", cnt: ");
        uart_puts(itoa(cnt));
        uart_puts(", ptr: ");
        uart_hex(&cnt);
        uart_puts(", sp: ");
        uart_hex(cur_sp);
        uart_puts("\r\n");

        ++cnt;

        if ((ret = fork()) != 0){
            asm volatile("mov %0, sp" : "=r"(cur_sp));

            uart_puts("first child pid: ");
            uart_puts(itoa(get_pid()));
            uart_puts(", cnt: ");
            uart_puts(itoa(cnt));
            uart_puts(", ptr: ");
            uart_hex(&cnt);
            uart_puts(", sp: ");
            uart_hex(cur_sp);
            uart_puts("\r\n");
        }
        else{
            while (cnt < 5) {
                asm volatile("mov %0, sp" : "=r"(cur_sp));
                
                uart_puts("second child pid: ");
                uart_puts(itoa(get_pid()));
                uart_puts(", cnt: ");
                uart_puts(itoa(cnt));
                uart_puts(", ptr: ");
                uart_hex(&cnt);
                uart_puts(", sp: ");
                uart_hex(cur_sp);
                uart_puts("\r\n");

                delay(50000000);
                ++cnt;
            }
            exit();
        }
        exit();
    }
    else {
        uart_puts("parent here, pid ");
        uart_puts(itoa(get_pid()));
        uart_puts(", child ");
        uart_puts(itoa(ret));
        uart_puts("\r\n");
    }
    exit();
}

void test_syscall() {
    /******** PID ********/
    // int pid = get_pid();
    // uart_puts("sys_getpid: ");
    // uart_puts(itoa(pid));
    // uart_puts("\n");

    /******** Read & Write ********/
    // char buf[32];
    // int ret = uart_read(buf, 10);
    // uart_puts("sys_uart_read: ");
    // if (ret > 0) {
    //     uart_puts(buf);
    //     uart_puts("\n");
    // }
    // else {
    //     uart_puts("Failed to read from UART\n");
    // }
    // ret = uart_write(buf, 10);
    // if (ret > 0) {
    //     uart_puts("sys_uart_write: ");
    //     uart_puts(buf);
    //     uart_puts("\n");
    // }
    // else {
    //     uart_puts("Failed to write to UART\n");
    // }
    // _exec();

    /******** Fork ********/
    struct ThreadTask* new_thread = thread_create(fork_test, PAGE_SIZE);

    asm volatile(
        "msr tpidr_el1, %0\n"
        "mov x5, 0x0\n"
        "msr spsr_el1, x5\n"
        "msr elr_el1, %1\n"
        "msr sp_el0, %2\n"
        "mov sp, %3\n"
        "eret"
        :
        : "r"(new_thread), "r"(new_thread->cpu_context.lr), "r"(new_thread->cpu_context.sp), 
          "r"(new_thread->kernel_stack + THREAD_STACK_SIZE)
        : "x5"
    );
}

// Create a shell thread that run in EL0
void create_shell_thread() {
    uart_puts("\nCreating shell thread...\n");
    struct ThreadTask *shell_thread = (struct ThreadTask *)alloc(sizeof(struct ThreadTask));
    if (shell_thread == NULL) {
        uart_puts("Failed to allocate memory for shell!\n");
        return -1;
    }

    // Initialize the task
    shell_thread->id = thread_cnt++;
    shell_thread->state = TASK_READY;
    shell_thread->counter = DEFAULT_PRIORITY;
    shell_thread->priority = DEFAULT_PRIORITY;
    shell_thread->preempt_count = 1;
    shell_thread->text = shell; // The shell function
    shell_thread->text_size = 0; // Size is not used for shell
    shell_thread->kernel_stack = alloc(THREAD_STACK_SIZE);
    if (shell_thread->kernel_stack == NULL) {
        uart_puts("Failed to allocate memory for shell stack!\n");
        free(shell_thread);
        return NULL;
    }
    shell_thread->user_stack = alloc(THREAD_STACK_SIZE * 3);
    if (shell_thread->user_stack == NULL) {
        uart_puts("Failed to allocate memory for shell stack!\n");
        free(shell_thread->kernel_stack);
        free(shell_thread);
        return NULL;
    }

    // Initialize signal handling
    shell_thread->pending_sig = 0;
    for (int i = 0; i < SIG_NUM; i++) {
        if (i == SIGKILL) shell_thread->sig_handlers[i] = default_sigkill_handler;
        else shell_thread->sig_handlers[i] = default_handler;
    }
    shell_thread->sig_frame = (struct TrapFrame *)alloc(sizeof(struct TrapFrame));
    shell_thread->next = NULL;

    // Initialize the memory management structure
    // shell_thread->mm = (struct mm_struct *)alloc(sizeof(struct mm_struct));
    // if (shell_thread->mm == NULL) {
    //     uart_puts("Failed to allocate memory for mm_struct!\n");
    //     free(shell_thread->kernel_stack);
    //     free(shell_thread->user_stack);
    //     free(shell_thread);
    //     return NULL;
    // }

    // shell_thread->mm->pgd = 0x0;
    
    uart_puts("user stack: ");
    uart_hex_long((unsigned long)shell_thread->user_stack);
    uart_puts(", kernel stack: ");
    uart_hex_long((unsigned long)shell_thread->kernel_stack);
    uart_puts("\n");

    memset((void*)&shell_thread->cpu_context, 0, sizeof(struct cpu_context));
    shell_thread->cpu_context.lr = (unsigned long)shell; // Set the entry point of the task
    shell_thread->cpu_context.sp = (unsigned long)shell_thread->user_stack + THREAD_STACK_SIZE - 0x10;
    shell_thread->cpu_context.fp = shell_thread->cpu_context.sp;

    // Add the task to the ready queue
    add_thread_task(&ready_queue, shell_thread);

    
    // eret 之後 sp 怪怪的
    asm volatile(
        "msr tpidr_el1, %0\n"
        "mov x5, 0x0\n"
        "msr spsr_el1, x5\n"
        "msr elr_el1, %1\n"
        "msr sp_el0, %2\n"
        "mov sp, %3\n"
        "eret"
        :
        : "r"(shell_thread), "r"(shell_thread->cpu_context.lr), "r"(shell_thread->cpu_context.sp), 
          "r"(shell_thread->kernel_stack + THREAD_STACK_SIZE)
        : "x5"
    );
}

void main() {
    // Get the address of the device tree blob
    uint32_t dtb_address;
    asm volatile ("mov %0, x20" : "=r" (dtb_address));

    int ret = fdt_init((void*)dtb_address);
    if (ret) {
        uart_puts("Failed to initialize the device tree blob!\n");
        return;
    }
    ret = fdt_traverse(initramfs_callback);
    if (ret) {
        uart_puts("Failed to traverse the device tree blob!\n");
        return;
    }

    mm_init();
    reserve(0x0000, 0x5000);                                    // Spin tables for multicore boot + PGD & PUD's page frame
    reserve((void*)&kernel_start, (void*)&__stack_top);         // Kernel image & startup allocator
    reserve((void*)VIRT_TO_PHYS(cpio_addr), (void*)VIRT_TO_PHYS(cpio_end));                 // Initramfs
    reserve((void*)dtb_address, (void*)dtb_address + be2le_u32(fdt_total_size));   // Devicetree

    kmem_cache_init();

    enable_irq_el1();

    sched_init();

    timer_init();

    // Lab5 Basic 1: Threads
    // sched_init();
    // for(int i = 0; i < 5; ++i) { // N should > 2
    //     thread_create(foo);
    // }
    // idle();

    // Lab5 Basic 2: Fork
    // thread_create(test_syscall);

    // create_shell_thread();
    _exec("vm.img");
    
    // Lab3 Basic 2: Print uptime every 2 seconds
    // add_timer(print_uptime, NULL, 2 * get_freq());

    // shell();
}