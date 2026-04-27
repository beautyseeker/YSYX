#include <memory.h>

static void *pf = NULL;

void* new_page(size_t nr_page) {
  void *old_pf = pf;
  pf += nr_page * PGSIZE;
  memset(old_pf, 0, nr_page * PGSIZE);
  if(pf > (void *)heap.end) {
    panic("Out of physical memory! pf (%p) exceeded heap.end (%p)", pf, heap.end);
  }
  // Assert(old_pf != NULL, "Failed to allocate new page");
  return old_pf;
}

#ifdef HAS_VME
static void* pg_alloc(int n) {
  void *page_paddr = new_page(n);
  Assert(page_paddr != NULL, "Failed to allocate physical page");
  return page_paddr;
}
#endif

void free_page(void *p) {
  panic("not implement yet");
}

/* The brk() system call handler. */
int mm_brk(uintptr_t brk) {
  return 0;
}

void init_mm() {
  pf = (void *)ROUNDUP(heap.start, PGSIZE);
  Log("free physical pages starting from %p", pf);

#ifdef HAS_VME
  vme_init(pg_alloc, free_page);
#endif
}
