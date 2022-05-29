// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define NBUCKET 13  // number of buckets for bcache

struct {
  struct spinlock lock_bcache;
  struct spinlock lock_bucket[NBUCKET];
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct buf bucket[NBUCKET];
} bcache;

void
binit(void)
{
  // init the bcache locks
  initlock(&bcache.lock_bcache, "bcache");
  for (int i = 0; i < NBUCKET; i++)
  {
    initlock(&bcache.lock_bucket[i], "bcache.bucket");
  }

  // create hash bucket by looping through buf
  for (int i = 0; i < NBUF; i++)
  {
    int index = i%NBUCKET;
    bcache.buf[i].next = bcache.bucket[index].next;
    bcache.bucket[index].next = &bcache.buf[i];
    initsleeplock(&bcache.buf[i].lock, "buffer");
  }
}

static struct buf*
findlsttimebufinbucket(int index)
{
  uint lst_time = 0xffffffff;
  struct buf *b_prev = &bcache.bucket[index];
  struct buf *b = bcache.bucket[index].next;
  struct buf *b_lst_time = 0;
  struct buf *b_lst_time_prev = 0;
  int find = 0;

  // find the buf with least time
  while (b)
  {
    if (b->refcnt == 0 && b->time < lst_time)
    {
      b_lst_time_prev = b_prev;
      b_lst_time = b;
      lst_time = b->time;
      find = 1;
    }
    b_prev = b;
    b = b->next;
  }

  if(find){
    b_lst_time_prev->next = b_lst_time->next;
    return b_lst_time;
  }
  else{
    return 0;
  }
}

static void
insert2head(struct buf* head, struct buf* value)
{
  value->next = head->next;
  head->next = value;
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int index = blockno%NBUCKET;

  // Is the block already cached? look from head to next
  // in case one thread found nothing while other thread happen to insert this block buf
  acquire(&bcache.lock_bucket[index]);
  b = bcache.bucket[index].next;
  while (b)
  {
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock_bucket[index]);
      acquiresleep(&b->lock);
      return b;
    }
    b = b->next;
  }
  release(&bcache.lock_bucket[index]);  // since no two processes operate the same block

  // if no block already, look for ref == 0 buf in its own bucket
  acquire(&bcache.lock_bucket[index]);
  b = findlsttimebufinbucket(index);
  release(&bcache.lock_bucket[index]);

  // if no ref==0 in its own bucket, look for ref == 0 in other buckets
  int nindex = (index+1)%NBUCKET;
  while (!b && nindex != index)
  {
    acquire(&bcache.lock_bucket[nindex]);
    b = findlsttimebufinbucket(nindex);
    release(&bcache.lock_bucket[nindex]);
    nindex = (nindex+1)%NBUCKET;
  }
  
  if(b)
  {
    acquire(&bcache.lock_bucket[index]);
    insert2head(&bcache.bucket[index], b);
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    release(&bcache.lock_bucket[index]);
    acquiresleep(&b->lock);
    return b;
  }
  else
  {
    panic("bget: no buffers");
  }
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  // printf(">>> read in bucket index %d\n", blockno%NBUCKET);
  b = bget(dev, blockno);

  // when it is a new buffer and needs to be read from disk
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  // printf(">>> write in bucket index %d\n", (b->blockno)%NBUCKET);
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  int index = b->blockno%NBUCKET;
  acquire(&bcache.lock_bucket[index]);
  b->refcnt--;
  b->time = ticks;
  release(&bcache.lock_bucket[index]);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock_bcache);
  b->refcnt++;
  release(&bcache.lock_bcache);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock_bcache);
  b->refcnt--;
  release(&bcache.lock_bcache);
}


