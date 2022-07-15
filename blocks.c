// based on cs3650 starter code

#define _GNU_SOURCE
#include <string.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "bitmap.h"
#include "blocks.h"

static int blocks_fd = -1;
static void *blocks_base = 0;

// Get the number of blocks needed to store the given number of bytes.
int bytes_to_blocks(int bytes) {
  int quo = bytes / BLOCK_SIZE;
  int rem = bytes % BLOCK_SIZE;
  if (rem == 0) {
    return quo;
  } else {
    return quo + 1;
  }
}

// Load and initialize the given disk image.
void blocks_init(const char *image_path) {
  blocks_fd = open(image_path, O_CREAT | O_RDWR, 0644);
  assert(blocks_fd != -1);

  // make sure the disk image is exactly 1MB
  int rv = ftruncate(blocks_fd, NUFS_SIZE);
  assert(rv == 0);

  // map the image to memory
  blocks_base =
      mmap(0, NUFS_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, blocks_fd, 0);
  assert(blocks_base != MAP_FAILED);

  // block 0 stores the block bitmap and the inode bitmap
  void *bbm = get_blocks_bitmap();
  bitmap_put(bbm, 0, 1);
}

// Close the disk image.
void blocks_free() {
  int rv = munmap(blocks_base, NUFS_SIZE);
  assert(rv == 0);
}

// Get the given block, returning a pointer to its start.
void *blocks_get_block(int bnum) { return blocks_base + BLOCK_SIZE * bnum; }

// Return a pointer to the beginning of the block bitmap.
// The size is BLOCK_BITMAP_SIZE bytes.
void *get_blocks_bitmap() { return blocks_get_block(0); }

// Return a pointer to the beginning of the inode table bitmap.
void *get_inode_bitmap() {
  uint8_t *block = blocks_get_block(0);

  // The inode bitmap is stored immediately after the block bitmap
  return (void *) (block + BLOCK_BITMAP_SIZE);
}

// Allocate a new block and return its index.
int alloc_block() {
  void *bbm = get_blocks_bitmap();

  for (int ii = 1; ii < BLOCK_COUNT; ++ii) {
    if (!bitmap_get(bbm, ii)) {
      bitmap_put(bbm, ii, 1);
      printf("+ alloc_block() -> %d\n", ii);

      reset_next_block_i(ii);
      return ii;
    }
  }

  return -1;
}

// Deallocate the block with the given index.
void free_block(int bnum) {
  printf("+ free_block(%d)\n", bnum);
  void *bbm = get_blocks_bitmap();
  bitmap_put(bbm, bnum, 0);
}


//The following code was added to support extension blocks, in which
//the last sieof(int) space is reserved for pointing towards an extension
//block if it is needed. By default, this int is always reset to -1


int block_data_size(){
  return BLOCK_SIZE - sizeof(int);
}

int get_next_block_i(int block_i){
  int *next_block_i_pntr = (int *) (blocks_get_block(block_i) + BLOCK_SIZE - sizeof(int));

  return next_block_i_pntr[0];
}

int reset_next_block_i(int block_i){
  int *next_block_i_pntr = (int *) (blocks_get_block(block_i) + BLOCK_SIZE - sizeof(int));
  *next_block_i_pntr = -1;
  return 0;
}

int allocate_extension_block(int block_i){
  int new_block_i = alloc_block();
  
  int *next_block_i_pntr = (int *) (blocks_get_block(block_i) + BLOCK_SIZE - sizeof(int));
  *next_block_i_pntr = new_block_i;
  return new_block_i;
}

int get_last_ext_block_i(int block_i){
  int curr_block_i = block_i;
  int prev_block_i = block_i;
  void *bbm = get_blocks_bitmap();

  while (curr_block_i > 0 && bitmap_get(bbm, curr_block_i) > 0){
    prev_block_i = curr_block_i;
    curr_block_i = get_next_block_i(curr_block_i);
  }

  return prev_block_i;
}
