// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NPROC]; // define the struct variable kmem at the same time.

// only one cpu called kinit() at the beginning
void
kinit()
{
  // uint64 pa_start = PGROUNDUP((uint64)end); // a start of the new page
  // uint64 npages = (PHYSTOP-pa_start)/PGSIZE;  // n pages in total
  // uint64 npages_cpu = npages/NPROC; // n pages each cpu

  // // TODO: should look for more eligant algorithm
  // // make each cpu has each own freelist
  // for (int i = 0; i < NPROC; i++)
  // {
  //   initlock(&kmem[i].lock, "kmem");
  //   if (i != NPROC-1){
  //     freerange(pa_start+i*npages_cpu*PGSIZE, pa_start+(i+1)*npages_cpu*PGSIZE);
  //   }
  //   else {
  //     freerange(pa_start+i*npages_cpu*PGSIZE, (void*)PHYSTOP);
  //   }
  // }

  for (int i = 0; i < NPROC; i++)
  {
    initlock(&kmem[i].lock, "kmem");
  }
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

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  // prevent process from being moved to other cpu by interrupt after already getting current cpuid
  // it would cause deadlock coz other cpu would never get kmem[current_cpu_id].lock
  push_off();
  int id = cpuid();
  acquire(&kmem[id].lock);
  r->next = kmem[id].freelist;
  kmem[id].freelist = r;
  release(&kmem[id].lock);
  pop_off();
}

struct run *
kalloc_cpu(int cpu_id)
{
  // printf(">>> cpu %d is tryint to take memory from cpu %d\n", cpuid(), cpu_id);
  struct run *r;
  acquire(&kmem[cpu_id].lock);
  r = kmem[cpu_id].freelist;
  if(r)
    kmem[cpu_id].freelist = r->next;
  release(&kmem[cpu_id].lock);
  return r;
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  // acquire this cpu's freelist by cpuid()
  push_off();
  int id = cpuid();
  r = kalloc_cpu(id);
  pop_off();

  // acquire from other cpu
  if(!r){
    for (int i = 0; i < NCPU; i++){
      r = kalloc_cpu(i);
      if (r) break;
    }
  }

  if(r)
    memset((char*)r, 5, PGSIZE); // fill with junk
  return (void*)r;
}
