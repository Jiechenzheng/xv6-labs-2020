// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

// reference count for each physical page
extern volatile int cow_refcnt[PHYSTOP/PGSIZE];

volatile int cow_refcnt[];

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

static void
rcinit()
{
  for(uint64 i = 0; i < PHYSTOP; i += PGSIZE)
  {
    cow_refcnt[i/PGSIZE] = 0;
  }
}

void
kinit()
{
  rcinit();
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  // do not free the page when the ref >= 1
  cow_refcnt[(uint64)pa/PGSIZE]--;
  if(cow_refcnt[(uint64)pa/PGSIZE] > 0)
    return;

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk

  cow_refcnt[(uint64)r/PGSIZE] = 1;
  return (void*)r;
}

uint64
cowalloc(pagetable_t pagetable, uint64 va0)
{
  uint64 pa0, pa0_old;
  uint flags;
  pte_t *pte;
  struct proc *p = myproc();

  pa0 = walkaddr(pagetable, va0);

  if(pa0 == 0)
    return 0;

  if(cow_refcnt[pa0/PGSIZE] > 1){
    // allocate new page
    pa0_old = pa0;
    pa0 = (uint64)kalloc();
    if(pa0 == 0){
      p->killed = 1;
      exit(-1);
    }

    // copy old page
    memmove((char*)pa0, (char*)pa0_old, PGSIZE);

    // unmap old pa and map a new pa
    pte = walk(pagetable, va0, 0);
    flags = PTE_FLAGS(*pte);
    flags = (flags & (~PTE_COW)) | PTE_W;
    uvmunmap(pagetable, va0, 1, 1);
    if(mappages(pagetable, va0, PGSIZE, pa0, flags) != 0){
      kfree((void*)pa0);
      p->killed = 1;
      exit(-1);
    }

    return pa0;
  }

  if(cow_refcnt[pa0/PGSIZE] == 1)
  {
    pte = walk(pagetable, va0, 0);
    flags = PTE_FLAGS(*pte);
    flags = (flags & (~PTE_COW)) | PTE_W;
    *pte = (*pte) | flags;

    return pa0;
  }

  return pa0;
}
