#include <defs.h>
#include <stdio.h>
#include <screen.h>
#include <sys/init_desc_table.h>
#include <sys/irq_common.h>
#include <sys/paging.h>
#include <sys/phys_mm.h>
#include <sys/virt_mm.h>
#include <sys/kmalloc.h>
//#include <sys/proc_mngr.h>

#define K_MEM_PAGES 518
#define INITIAL_STACK_SIZE 4096

char stack[INITIAL_STACK_SIZE];
uint32_t* loader_stack;
extern char kernmem, physbase;

void fun1(void)
{
    int i = 0;
    while(i < 10){
        kprintf("\n%d In fun1()", i++);
    }
    kprintf("\nOut of fun1()");
    while(1);
}

void fun2(void)
{
    int i = 0;
    while(i < 10){
        kprintf("\n%d In fun2()", i++);
    }
    kprintf("\nOut of fun2()");
    while(1);
}
        
void start(uint32_t* modulep, void* physbase, void* physfree)
{
    uint64_t phys_size = 0x0, phys_base = 0x0;

    struct smap_t {
        uint64_t base, length;
        uint32_t type;
    }__attribute__((packed)) *smap;
    while(modulep[0] != 0x9001) modulep += modulep[1]+2;
    for(smap = (struct smap_t*)(modulep+2); smap < (struct smap_t*)((char*)modulep+modulep[1]+2*4); ++smap) {
        if (smap->type == 1 /* memory */ && smap->length != 0) {
            set_cursor_pos(15, 5);
            kprintf("Available Physical Memory [%x-%x]\n", smap->base, smap->base + smap->length);
            phys_size = smap->length;
            phys_base = smap->base;
        }
    }
   
    // kernel starts here
    
    set_cursor_pos(16, 5);
    kprintf("PhysBase = %p PhysFree = %p", physbase, physfree);

    // Start physical memory at 4MB
    phys_init((phys_size - 0x300000)/8192, phys_base + 0x300000); 

    init_paging((uint64_t)&kernmem, (uint64_t)physbase, K_MEM_PAGES);

// TODO: Context Switching code
#if 0
    // Setup the stack again 
    __asm__ __volatile__("movq %0, %%rbp" : :"a"(&stack[0]));
    __asm__ __volatile__("movq %0, %%rsp" : :"a"(&stack[INITIAL_STACK_SIZE]));

    task_struct* struct1 = (task_struct*)kmalloc(sizeof(task_struct));
    task_struct* struct2 = (task_struct*)kmalloc(sizeof(task_struct));
    create_new_process(struct1, (uint64_t)fun1);
    create_new_process(struct2, (uint64_t)fun2);
#endif

    // Allow interrupts
    __asm__ __volatile__("sti");

    kprintf("\nEnd of Kernel");

    while(1);
}


void boot(void)
{
    // note: function changes rsp, local stack variables can't be practically used
    register char *temp1, *temp2;
    __asm__(
            "movq %%rsp, %0;"
            "movq %1, %%rsp;"
            :"=g"(loader_stack)
            :"r"(&stack[INITIAL_STACK_SIZE])
           );

    // Disable interrupts
    __asm__ __volatile__("cli");

    // Intialize
    init_gdt();
    init_idt();
    init_pic();
    init_tss();
    init_timer(1);
    init_keyboard();

    init_screen(0xFFFFFFFF800B8000);
    clear_screen();

    set_color(RED, BLACK);
    set_cursor_pos(11, 25);
    kprintf("________________");
    set_cursor_pos(12, 25);
    kprintf("|    SBUnix    |");
    set_cursor_pos(13, 25);
    kprintf("----------------");

    start(
            (uint32_t*)((char*)(uint64_t)loader_stack[3] + (uint64_t)&kernmem - (uint64_t)&physbase),
            &physbase,
            (void*)(uint64_t)loader_stack[4]
         );
    for(
            temp1 = "!!!!! start() returned !!!!!", temp2 = (char*)0xb8000;
            *temp1;
            temp1 += 1, temp2 += 2
       ) *temp2 = *temp1;
    while(1);
}

