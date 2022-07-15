// based on cs3650 starter code

#ifndef BLOCKS_H
#define BLOCKS_H

#include <stdio.h>

//Hervella changes: made these define instead of const int to share amongst files
#define BLOCK_COUNT 256 // we split the "disk" into blocks (default = 256)
#define BLOCK_SIZE 4096 // default = 4K
#define NUFS_SIZE BLOCK_SIZE * BLOCK_COUNT  // default = 1MB

#define BLOCK_BITMAP_SIZE BLOCK_COUNT / 8  // default = 256 / 8 = 32

// Get the number of blocks needed to store the given number of bytes.
int bytes_to_blocks(int bytes);

// Load and initialize the given disk image.
void blocks_init(const char *image_path);

// Close the disk image.
void blocks_free();

// Get the block with the given index, returning a pointer to its start.
void *blocks_get_block(int bnum);

// Return a pointer to the beginning of the block bitmap.
void *get_blocks_bitmap();

// Return a pointer to the beginning of the inode table bitmap.
void *get_inode_bitmap();

// Allocate a new block and return its index.
int alloc_block();

// Deallocate the block with the given index.
void free_block(int bnum);

//Retrurns the actual size if we account for the last sizeof(int) space
//reserved at the end of the block for the block index referencing the next block
int block_data_size();

//Returns the next block index (stored at the end of each block) if successful, -1 if not available
//or on failure
int get_next_block_i(int block_i);

//Resets the end of the block where the next block index is stored to -1
//Returns 0 on success, -1 on failure
int reset_next_block_i(int block_i);

//Alloactes an extension block based on the block index given
//Returns the new extension block index on success, -1 on failure
int allocate_extension_block(int block_i);

//Iterates through the given block index until the last extension block is reached
//Returns the last block available (inclusive the initial block given) on success, -1 on failure
int get_last_ext_block_i(int block_i);
#endif
