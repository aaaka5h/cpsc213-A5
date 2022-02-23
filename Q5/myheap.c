#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

#include "myheap.h"

#define HEADER_SIZE 8

/*
 * Struct used to represent the heap.
 */
struct myheap {
    long size;                /* Size of the heap in bytes. */
    void *start;             /* Start address of the heap area. */
};

/*
 * Determine whether or not a block is in use.
 */
static int block_is_in_use(void *block_start) {

  return 1 & *((long *) block_start);
}

/*
 * Return the size of a block.
 */
static int get_block_size(void *block_start) {

  long *header = block_start;
  // remove the last bit from header (i.e, the alloc bit) and return the result (i.e., the block size)
  return *header & 0xfffffffffffffffe;
}

/*
 * Return the size of the payload of a block.
 */
static int get_payload_size(void *block_start) {
  
  return get_block_size(block_start) - HEADER_SIZE * 2;
}

/*
 * Find the start of the block, given a pointer to the payload.
 */
static void *get_block_start(void *payload) {
  
  return payload - HEADER_SIZE;
}

/*
 * Find the payload, given a pointer to the start of the block.
 */
static void *get_payload(void *block_start) {
  
  return block_start + HEADER_SIZE;
}

/*
 * Set the size of a block, and whether or not it is in use. Remember
 * each block has two copies of the header (one at each end).
 */
static void set_block_header(void *block_start, int block_size, int in_use) {
  
  long header_value = block_size | in_use;
  long *header_position = block_start;
  long *trailer_position = block_start + block_size - HEADER_SIZE;
  *header_position  = header_value;
  *trailer_position = header_value;
}


/*
 * Find the start of the next block.
 */
static void *get_next_block(void *block_start) {
  
  return block_start + get_block_size(block_start);
}

/*
 * Find the start of the previous block.
 */
static void *get_previous_block(void *block_start) {
  
  return block_start - get_block_size(block_start - HEADER_SIZE);
}

/*
 * Determine whether or not the given block is at the front of the heap.
 */
static int is_first_block(struct myheap *h, void *block_start) {
  
  return block_start == h->start;
}

/*
 * Determine whether or not the given block is at the end of the heap.
 */
static int is_last_block(struct myheap *h, void *block_start) {
  
  return get_next_block(block_start) == h->start + h->size;
}

/*
 * Determine whether or not the given address is inside the heap
 * region. Can be used to loop through all blocks:
 *
 * for (blk = h->start; is_within_heap_range(h, blk); blk = get_next_block(blk)) ...
 */
static int is_within_heap_range(struct myheap *h, void *addr) {
  
  return addr >= h->start && addr < h->start + h->size;
}

/*
 * Coalesce (joins) a block with its consecutive (next) block, but
 * only if both blocks are free. Return a pointer to the beginning of
 * the coalesced block.
 */
static void *coalesce(struct myheap *h, void *first_block_start) {
  
  if (block_is_in_use(first_block_start) == 1 || is_last_block(h, first_block_start)) {
    return first_block_start;
  } else {
    void* next_block = get_next_block(first_block_start);
    if (block_is_in_use(first_block_start) == 0 && block_is_in_use(next_block) == 0 && is_within_heap_range(h, next_block)) {
      int calesced_size = get_block_size(first_block_start) + get_block_size(next_block);
      set_block_header(first_block_start, calesced_size, 0);
    }
      return first_block_start;
  }
}

/*
 * Determine the size of the block we need to allocate given the size
 * the user requested. Don't forget we need space for the header and
 * footer, and that the block's actual payload size must be a multiple
 * of HEADER_SIZE.
 */
static int get_size_to_allocate(int user_size) {
  
  /* TO BE COMPLETED BY THE STUDENT. */
  int remainder = HEADER_SIZE - (user_size % HEADER_SIZE);
  if (remainder == HEADER_SIZE) {
    remainder = 0;
  }
  return HEADER_SIZE + user_size + remainder + HEADER_SIZE;
}

/*
 * Checks if the block can be split. It can split if the left over
 * bytes after the split (current block size minus needed size) are
 * enough for a new block, i.e., there is enough space left over for a
 * header, trailer and some payload (i.e., at least 3 times
 * HEADER_SIZE). If it can be split, splits the block in two and marks
 * the first as in use, the second as free. Otherwise just marks the
 * block as in use. Returns the payload of the block marked as in use.
 */
static void *split_and_mark_used(struct myheap *h, void *block_start, int needed_size) {

  /* TO BE COMPLETED BY THE STUDENT. */
  if ((get_block_size(block_start) - needed_size >= HEADER_SIZE * 3) && is_within_heap_range(h, block_start + needed_size)) {
    void* free_block = block_start + needed_size;
    set_block_header(free_block, get_block_size(free_block) - needed_size, 0);
    set_block_header(block_start, needed_size, 1);
    return get_payload(block_start);
  }
    set_block_header(block_start, get_block_size(block_start), 1);
    return get_payload(block_start);
}

/*
 * Create a heap that is "size" bytes large.
 */
struct myheap *heap_create(unsigned int size)
{
  /* Allocate space in the process' actual heap */
  void *heap_start = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (heap_start == (void *) -1) return NULL;
  
  /* Use the first part of the allocated space for the heap header */
  struct myheap *h = heap_start;
  h->size = size - sizeof(struct myheap);
  h->start = heap_start + sizeof(struct myheap);

  /* Initializes one big block with the full space in the heap. Will
     be split in the first malloc. */
  set_block_header(h->start, h->size, 0);
  return h;
}

/*
 * Free a block on the heap h. Also attempts to join (coalesce) the
 * block with the previous and the next block, if they are also free.
 */
void myheap_free(struct myheap *h, void *payload) {
  
  /* TO BE COMPLETED BY THE STUDENT. */
  void* block_start = get_block_start(payload);
  int block_size = get_block_size(block_start);
  set_block_header(block_start, block_size, 0);

  void* prev_block = get_previous_block(block_start);
  coalesce(h, block_start);
  if (!is_first_block(h, block_start)) {
    coalesce(h, prev_block);
  }
}

/*
 * Malloc a block on the heap h. 
 * Return a pointer to the block's payload in case of success, 
 * or NULL if no block large enough to satisfy the request exists.
 */
void *myheap_malloc(struct myheap *h, unsigned int user_size) {
  
  /* TO BE COMPLETED BY THE STUDENT. */
  int size_needed = get_size_to_allocate(user_size);
  for (void* blk = h->start; is_within_heap_range(h, blk); blk = get_next_block(blk)) {
    if (block_is_in_use(blk) == 0 && size_needed <= get_block_size(blk)) {
      return get_payload(blk);
    }
  }
  return NULL;
}
