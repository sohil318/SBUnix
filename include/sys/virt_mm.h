#include <defs.h>

uint64_t get_top_virtaddr();
void set_top_virtaddr(uint64_t vaddr);

void* virt_alloc_pages (uint32_t no_of_vpages);
void free_virt_page(void *vaddr);

