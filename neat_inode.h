#ifndef NEAT_INODE_H
#define NEAT_INODE_H

#include <time.h>
#include "blocks.h"

#define INODE_BITMAP_SIZE BLOCK_SIZE - BLOCK_BITMAP_SIZE
//#define MAX_INODE //INODE_BITMAP_SIZE / 8 // should round down b/c divinding by an int

typedef struct neat_inode {
    int size;
    int mode;
    int block_i;
    int inode_i;
    
    time_t ctime;
    time_t mtime;
    time_t atime;
} neat_inode_t;

//Initialzie the inode bitmap
//Rerturn 0 on success, -1 on error
int inode__init_inode_block();

//Prints inode info
//Returns 0 on success, 1 on error
//int inode_print_inode();

//Gets the inode based of the inode index
//Returns the inode on success, null on failure
neat_inode_t *inode__get_inode(int inode_i);

//Gets the inode count (currently a calculated max)
//Returns the count on success, 1 on failure
int inode__get_inode_count();

//Get the inode pointer base
//Returns the pointer on success, null on failure
void *inode__get_inode_base();

//Allocates an inode in inode block region, marks inode bitmap
//returns the inode, NULL on error
neat_inode_t *inode__alloc_inode();

//Mark it as freed in the bitmap (free_block) for all blocks used
//Returns 0 on success, -1 on failure
int inode__free_inode(int inode_i);

//Grow the size of the inode, and if too big for block,
//mark the end of the block with the block index of the next one used (recursively)
//Returns 0 on success, 1 on failure
int inode__grow_inode(neat_inode_t *inode, int size);

//Shrink the size
//Returns 0 on success, 1 on failure
int inode__shrink_inode(neat_inode_t *inode, int size);

//Gets the base of where base of the pntr (accomodating for space the inode itself takes)
//Return the base pntr on success
void *inode__get_data_base_pntr(int inode_i);
#endif