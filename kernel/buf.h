/*
 * use single linked list to simplify the data structure
 */
struct buf {
  int valid;   // has data been read from disk? Yes if the buffer contians a copy of the block
  int disk;    // does disk "own" buf?
  uint dev;
  uint blockno;
  struct sleeplock lock;
  uint refcnt;
  struct buf *prev;
  struct buf *next;
  uint time;
  uchar data[BSIZE];
};

