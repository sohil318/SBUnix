#include <stdio.h>
#include <sys/proc_mngr.h>
#include <sys/paging.h>
#include <sys/virt_mm.h>
#include <sys/tarfs.h>
#include <sys/elf.h>
#include <sys/kmalloc.h>
#include <string.h>

#define USER_STACK_ADDR 0xF0000000

uint64_t next_pid = 0;

void increment_brk(task_struct *proc, uint64_t bytes)
{   
    //kprintf("\n new heapend: %p", addr);
    mm_struct *mms = proc->mm;
    vma_struct *iter;
    
    for (iter = mms->vma_list; iter != NULL; iter = iter->vm_next) {
        //kprintf("\n vm_start %p\t start_brk %p\t end_brk %p", iter->vm_start, mms->start_brk, mms->end_brk);

        // this vma is pointing to heap
        if (iter->vm_start == mms->start_brk) {
            iter->vm_end  += bytes;
            mms->end_brk  += bytes; 
            mms->total_vm += bytes;
            break;
        }
    }
}

vma_struct* alloc_new_vma(uint64_t start_addr, uint64_t end_addr)
{
    vma_struct *vma = (vma_struct*) kmalloc(sizeof(vma_struct));
    vma->vm_start   = start_addr;
    vma->vm_end     = end_addr; 
    vma->vm_next    = NULL;
    return vma;
}

task_struct* alloc_new_task(bool IsUserProcess)
{
    mm_struct* mms;
    task_struct* new_proc;

    // Allocate a new mm_struct
    mms = (mm_struct *) kmalloc(sizeof(mm_struct));
    mms->pml4_t     = create_new_pml4();
    mms->vma_list   = NULL;
    mms->vma_count  = NULL;
    mms->hiwater_vm = NULL;
    mms->total_vm   = NULL;
    mms->stack_vm   = NULL;

    // Allocate a new task struct
    new_proc = (task_struct*) kmalloc(sizeof(task_struct));
    new_proc->pid           = next_pid++;
    new_proc->ppid          = 0;
    new_proc->IsUserProcess = IsUserProcess;
    new_proc->mm            = mms;
    new_proc->next          = NULL;
    new_proc->last          = NULL;
    new_proc->parent        = NULL;
    new_proc->children      = NULL;
    new_proc->sibling       = NULL;

#if DEBUG_SCHEDULING
    kprintf("\nPID:%d\tCR3: %p", new_proc->pid, mms->pml4_t);
#endif

    return new_proc;
}

void* mmap(uint64_t start_addr, int bytes)
{
    int no_of_pages = 0;
    uint64_t end_vaddr;
    uint64_t cur_top_vaddr = get_top_virtaddr();

    // Use new Vaddr
    set_top_virtaddr(PAGE_ALIGN(start_addr));

    // Find no of pages to be allocated
    end_vaddr = start_addr + bytes;
    no_of_pages = (end_vaddr >> 12) - (start_addr >> 12) + 1;

    // Allocate VPages
    virt_alloc_pages(no_of_pages);

    // Restore old top Vaddr
    set_top_virtaddr(cur_top_vaddr);

    return (void*)start_addr;
}

// Loads CS and DS into VMAs and returns the entry point into process
uint64_t load_elf(Elf64_Ehdr* header, task_struct *proc)
{
    Elf64_Phdr* program_header;
    mm_struct *mms = proc->mm;
    vma_struct *node, *iter;
    uint64_t start_vaddr, end_vaddr, cur_pml4_t, max_addr = 0;
    int i, size;

    // Save current PML4 table
    READ_CR3(cur_pml4_t);

    // Offset at which program header table starts
    program_header = (Elf64_Phdr*) ((void*)header + header->e_phoff);  
    
    for (i = 0; i < header->e_phnum; ++i) {

        // kprintf("\n Type of Segment: %x", program_header->p_type);
        if ((int)program_header->p_type == 1) {           // this is loadable section

            start_vaddr = program_header->p_vaddr;
            size        = program_header->p_memsz;
 
            end_vaddr = start_vaddr + size;    
            node      = alloc_new_vma(start_vaddr, end_vaddr); 
            mms->vma_count++;
            mms->total_vm += size;

            //kprintf("\nstart:%p end:%p size:%p", start_vaddr, end_vaddr, size);

            if (max_addr < end_vaddr) {
                max_addr = end_vaddr;
            }
            
            // Load ELF sections into new Virtual Memory Area
            LOAD_CR3(mms->pml4_t);

            mmap(start_vaddr, size); 

            if (mms->vma_list == NULL) {
                mms->vma_list = node;
            } else {
                for (iter = mms->vma_list; iter->vm_next != NULL; iter = iter->vm_next);
                iter->vm_next = node;
            }

            //kprintf("\nVaddr = %p, ELF = %p, size = %p",(void*) start_vaddr, (void*) header + program_header->p_offset, size);
            memcpy((void*) start_vaddr, (void*) header + program_header->p_offset, program_header->p_filesz);

            // Set .bss section with zero
            // Note that, only in case of segment containing .bss will filesize and memsize differ 
            memset((void *)start_vaddr + program_header->p_filesz, 0, size - program_header->p_filesz);
            
            // Restore parent CR3
            LOAD_CR3(cur_pml4_t);
        }
        // Go to next program header
        program_header = program_header + 1;
    }
 
    // Traverse the vmalist to reach end vma and allocate a vma for the heap at 4k align
    for (iter = mms->vma_list; iter->vm_next != NULL; iter = iter->vm_next);
    start_vaddr = end_vaddr = ((((max_addr - 1) >> 12) + 1) << 12);
    iter->vm_next = alloc_new_vma(start_vaddr, end_vaddr); 

    mms->vma_count++;
    mms->start_brk = start_vaddr;
    mms->end_brk   = end_vaddr; 
    //kprintf("\tHeap Start:%p", mms->start_brk);

    // Stack VMA of one page (TODO: Need to allocate a dynamically growing stack)
    for (iter = mms->vma_list; iter->vm_next != NULL; iter = iter->vm_next);
    start_vaddr = USER_STACK_ADDR;
    end_vaddr = USER_STACK_ADDR + PAGESIZE;
    iter->vm_next = alloc_new_vma(start_vaddr, end_vaddr);

    // Map a physical page
    LOAD_CR3(mms->pml4_t);
    mmap(start_vaddr, PAGESIZE);
    LOAD_CR3(cur_pml4_t);

    mms->vma_count++;
    mms->stack_vm  = PAGESIZE;
    mms->total_vm += PAGESIZE;
    mms->start_stack = end_vaddr - 8;

    return header->e_entry;
}

pid_t create_elf_proc(char *filename)
{
    HEADER *header;
    Elf64_Ehdr* elf_header;
    task_struct* new_proc;
    uint64_t entrypoint;

    // lookup for the file in tarfs
    header = (HEADER*) lookup(filename); 
    
    if (header == NULL) {
        return -1;
    }

    // Check if file is an ELF executable by checking the magic bits
    elf_header = (Elf64_Ehdr *)header;
    
    if (elf_header->e_ident[1] == 'E' && elf_header->e_ident[2] == 'L' && elf_header->e_ident[3] == 'F') {                
        new_proc = alloc_new_task(TRUE);
        kstrcpy(new_proc->comm, filename);
        entrypoint = load_elf(elf_header, new_proc);
        schedule_process(new_proc, entrypoint, (uint64_t)new_proc->mm->start_stack);
        
        return new_proc->pid;
    }
    return -1;
}

