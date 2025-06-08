#include "sched.h"

struct ThreadTask *ready_queue = NULL;
struct ThreadTask *wait_queue = NULL;
struct ThreadTask *zombie_queue = NULL;

unsigned int thread_cnt = 0;

void print_queue(struct ThreadTask *queue) {
    struct ThreadTask *current = queue;
    while (current != NULL) {
        uart_puts(itoa(current->id));
        uart_puts("(");
        uart_hex((unsigned long)current);
        uart_puts(")");
        uart_puts(" -> ");
        current = current->next;
    }
    uart_puts("NULL\r\n");
}

void add_thread_task(struct ThreadTask **queue, struct ThreadTask *task) {
    // uart_puts("Adding task ");
    // uart_puts(itoa(task->id));
    // uart_puts(" to queue at ");
    // uart_hex((unsigned long)task);
    // uart_puts("\r\n");
    // Add the task to the ready queue
    if (*queue == NULL) {
        *queue = task;
    }
    else {
        struct ThreadTask *current = *queue;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = task;
    }
    task->next = NULL;

    // print_queue(*queue);
}

// Get the first task from the queue
struct ThreadTask* pop_thread_task(struct ThreadTask **queue) {
    if (*queue == NULL) {
        return NULL;
    }
    struct ThreadTask *task = *queue;
    *queue = (*queue)->next;
    return task;
}

void rm_thread_task(struct ThreadTask **queue, struct ThreadTask *task) {
    if (*queue == NULL) {
        return;
    }
    struct ThreadTask *current = *queue;
    struct ThreadTask *prev = NULL;

    while (current != NULL) {
        if (current == task) {
            if (prev == NULL) {
                *queue = current->next;
            }
            else {
                prev->next = current->next;
            }
            break;
        }
        prev = current;
        current = current->next;
    }
}

void sched_init() {
    ready_queue = NULL;
    wait_queue = NULL;
    zombie_queue = NULL;
    thread_cnt = 0;

    // Create a task for "idle"
    struct ThreadTask *idle_task = thread_create(idle, PAGE_SIZE);
    if (idle_task == NULL) {
        uart_puts("Failed to allocate memory for idle task!\n");
        return;
    }

    idle_task->state = TASK_RUNNING;
    set_current(idle_task);
}

struct ThreadTask* thread_create(void (*callback)(void), unsigned int filesize) {
    // Allocate memory for the task
    struct ThreadTask *task = (struct ThreadTask *)alloc(sizeof(struct ThreadTask));
    if (task == NULL) {
        uart_puts("Failed to allocate memory for task!\n");
        return -1;
    }

    // Initialize the task
    task->id = thread_cnt++;
    task->state = TASK_READY;
    task->counter = DEFAULT_PRIORITY;
    task->priority = DEFAULT_PRIORITY;
    task->preempt_count = 1;
    task->text = callback;
    task->text_size = filesize;
    task->kernel_stack = alloc(THREAD_STACK_SIZE);
    if (task->kernel_stack == NULL) {
        uart_puts("Failed to allocate memory for task stack!\n");
        free(task);
        return NULL;
    }
    task->user_stack = alloc(THREAD_STACK_SIZE * 3);
    if (task->user_stack == NULL) {
        uart_puts("Failed to allocate memory for task stack!\n");
        free(task->kernel_stack);
        free(task);
        return NULL;
    }

    // Initialize signal handling
    task->pending_sig = 0;
    for (int i = 0; i < SIG_NUM; i++) {
        if (i == SIGKILL) task->sig_handlers[i] = default_sigkill_handler;
        else task->sig_handlers[i] = default_handler;
    }
    task->sig_frame = (struct TrapFrame *)alloc(sizeof(struct TrapFrame));
    task->next = NULL;

    // Initialize the memory management structure
    // task->mm = (struct mm_struct *)alloc(sizeof(struct mm_struct));
    // if (task->mm == NULL) {
    //     uart_puts("Failed to allocate memory for mm_struct!\n");
    //     free(task->kernel_stack);
    //     free(task->user_stack);
    //     free(task);
    //     return NULL;
    // }

    memset((void*)&task->cpu_context, 0, sizeof(struct cpu_context));

    unsigned long *pgd_virt = (unsigned long *)alloc(PAGE_SIZE); // Allocate a page for the PGD
    memset(pgd_virt, 0, PAGE_SIZE);
    task->cpu_context.pgd = (unsigned long*)VIRT_TO_PHYS((unsigned long)pgd_virt);
    // uart_puts("Allocated PGD at: ");
    // uart_hex_long((unsigned long)task->cpu_context.pgd);
    // uart_puts("\n");
    
    // uart_puts("user stack: ");
    // uart_hex_long((unsigned long)task->user_stack);
    // uart_puts(", kernel stack: ");
    // uart_hex_long((unsigned long)task->kernel_stack);
    // uart_puts("\n");

    // uart_puts("Mapping user and kernel stacks for stacks\r\n");
    unsigned long flags = PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_TABLE | PD_USER_ACCESS/* | PD_UXN | PD_PXN*/;
    // uart_puts("Mapping user stack\r\n");
    map_pages(task->cpu_context.pgd, 0xffffffffb000, VIRT_TO_PHYS(task->user_stack),  3 * THREAD_STACK_SIZE, flags);
    // uart_puts("Mapping kernel stack\r\n");
    // flags &= ~PD_USER_ACCESS;   // Disable user access for kernel stack
    map_pages(task->cpu_context.pgd, 0xffffffffe000, VIRT_TO_PHYS(task->kernel_stack),  THREAD_STACK_SIZE, flags);
    // uart_puts("Mapping callback function\r\n");
    flags = PD_ACCESS | (MAIR_IDX_NORMAL_NOCACHE << 2) | PD_TABLE | PD_USER_ACCESS;
    map_pages(task->cpu_context.pgd, 0, VIRT_TO_PHYS((unsigned long)callback), filesize, flags);

    task->cpu_context.lr = (unsigned long)callback; // Set the entry point of the task
    task->cpu_context.sp = /*(unsigned long)task->user_stack + THREAD_STACK_SIZE - 0x10*/0xffffffffb000 + 3 * THREAD_STACK_SIZE - 0x10;
    task->cpu_context.fp = task->cpu_context.sp;

    // Add the task to the ready queue
    add_thread_task(&ready_queue, task);

    return task;
}

struct ThreadTask* get_thread_task_by_id(int pid) {
    struct ThreadTask *current = ready_queue;
    while (current != NULL) {
        if (current->id == pid) {
            return current;
        }
        current = current->next;
    }

    current = wait_queue;
    while (current != NULL) {
        if (current->id == pid) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

void _exit() {
    struct ThreadTask *curr = get_current();
    if (curr == NULL) return;

    rm_thread_task(&ready_queue, curr);
    rm_thread_task(&wait_queue, curr);

    curr->state = TASK_EXITED;

    schedule();  // Switch to the next task
}

int _kill(unsigned int pid) {
    struct ThreadTask *task = get_thread_task_by_id(pid);
    if (task == NULL) {
        uart_puts("[WARN] _kill: no running task with pid ");
        uart_puts(itoa(pid));
        uart_puts("\r\n");
        return -1;
    }

    rm_thread_task(&ready_queue, task);
    rm_thread_task(&wait_queue, task);

    task->state = TASK_EXITED;

    struct ThreadTask *curr = get_current();
    if (curr == task) schedule();
    else add_thread_task(&zombie_queue, task);

    return 0;
}

void schedule() {
    disable_irq_el1();
    timer_disable_irq();
    // uart_puts("Scheduling...\n");

    struct ThreadTask *prev = get_current();
    if (prev == NULL) {
        ready_queue->state = TASK_RUNNING;
        set_current(ready_queue);
    }
    else {
        // uart_puts("Previous ready queue: ");
        // print_queue(ready_queue);
        struct ThreadTask *next = ready_queue;

        if (next == NULL || next == prev) {  // No other task to switch to
            // uart_puts("No other task to switch to, staying in the current task.\n");
            enable_irq_el1();
            timer_enable_irq();
            return;
        }

        if (prev->state == TASK_RUNNING) {
            prev->state = TASK_READY;
            // uart_puts(itoa(prev->id));
            // uart_puts(" Running task\r\n");
            add_thread_task(&ready_queue, prev);
        }
        else if (prev->state == TASK_BLOCKED) {
            // uart_puts(itoa(prev->id));
            // uart_puts(" Blocking task\r\n");
            add_thread_task(&wait_queue, prev);
        }
        else if (prev->state == TASK_EXITED) {
            // uart_puts(itoa(prev->id));
            // uart_puts(" Exiting task\r\n");
            add_thread_task(&zombie_queue, prev);
        }
        else if (prev->state == TASK_READY) {
            // uart_puts(itoa(prev->id));
            // uart_puts(" Ready task\r\n");
        }
        else {
            uart_puts("Invalid thread state!\n");
            enable_irq_el1();
            timer_enable_irq();
            return;
        }

        // uart_puts("Current ready queue: ");
        // print_queue(ready_queue);
        
        // uart_puts("Switching from task ");
        // uart_puts(itoa(prev->id));
        // uart_puts(" (");
        // uart_hex((unsigned long)prev);
        // uart_puts(") to task ");
        // uart_puts(itoa(next->id));
        // uart_puts(" (");
        // uart_hex((unsigned long)next);
        // uart_puts(")\n");
        
        // Switch to the next task
        next->state = TASK_RUNNING;
        next = pop_thread_task(&ready_queue);
        // unsigned long next_pgd = (unsigned long)next->mm->pgd;
        // set_pgd(next_pgd);
        // uart_puts("next after set_pgd: ");
        // uart_hex_long(next);
        // uart_puts(", next->mm->pgd: ");
        // uart_hex_long((unsigned long)next->mm->pgd);
        // uart_puts("\n");
        // uart_puts("After set_pgd, next->mm->pgd: ");
        // uart_hex_long((unsigned long)next->mm->pgd);
        // uart_puts("\n");
        
        timer_enable_irq();
        // enable_irq_el1();
        
        cpu_switch_to(prev, next);
    }

    // timer_enable_irq();
    // enable_irq_el1();
    return;
}

void kill_zombies() {
    struct ThreadTask *zombie = pop_thread_task(&zombie_queue);
    while (zombie != NULL) {
        free(zombie->kernel_stack);
        free(zombie->user_stack);
        free(zombie);
        zombie = pop_thread_task(&zombie_queue);
    }
}

void idle() {
    while (1) {
        // uart_puts("Idle task running...\n");
        kill_zombies();
        schedule();
    }
}