#include <stdio.h>
#include <syscall.h>
#include <sys/proc_mngr.h>
#include <sys/virt_mm.h>
#include <io_common.h>
#include <string.h>

extern task_struct* CURRENT_TASK;

// These will get invoked in kernel mode

volatile int flag, counter;
volatile char buf[1024];

int sys_puts(char* s)
{
    puts(s);
    return 0;
}

int sys_gets(uint64_t addr)
{
    char *user_buf = (char*) addr;
    int count;

    flag = 1;
    sti;
    while(flag == 1);

    memcpy((void *)user_buf, (void *)buf, counter);
    count = counter;
    counter = 0;
    return count;
}

int sys_mmap(uint64_t size)
{
    uint64_t ret_addr = NULL;
    return ret_addr;
}

uint64_t sys_brk(uint64_t no_of_pages)
{
    uint64_t new_vaddr;
    uint64_t cur_top_vaddr = get_top_virtaddr();
    
    set_top_virtaddr(CURRENT_TASK->mm->end_brk);
    new_vaddr = (uint64_t)virt_alloc_pages(no_of_pages);
     
    //kprintf("\n New Heap Page Alloc:%p", new_vaddr);
    increment_brk(CURRENT_TASK, PAGESIZE * no_of_pages);

    // Restore old top Vaddr
    set_top_virtaddr(cur_top_vaddr);
    return new_vaddr;
}

pid_t sys_getpid()
{
    return CURRENT_TASK->pid;
}

pid_t sys_getppid()
{
    return CURRENT_TASK->ppid;
}

// Set up the system call table
void* syscall_tbl[NUM_SYSCALLS] = 
{
    sys_puts,
    sys_gets,
    sys_brk,
    sys_fork,
    sys_mmap,
    sys_getpid,
    sys_getppid
};

// The handler for the int 0x80
void syscall_handler(void)
{
    uint64_t syscallNo;

    __asm__ __volatile__("movq %%rax, %0;" : "=r"(syscallNo));

    if (syscallNo >= 0 && syscallNo < NUM_SYSCALLS) {
        void *func_ptr = syscall_tbl[syscallNo];
        uint64_t ret;

        __asm__ __volatile__("callq *%0;" : "=a" (ret) : "r" (func_ptr));
    }

    __asm__ __volatile__("iretq;");
}

