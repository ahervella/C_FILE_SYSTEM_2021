#include "blocks.h"
#include "neat_inode.h"
#include "neat_directory.h"
#include "bitmap.h"
#include <unistd.h>

#define INODE_FILE_NAME "neat_inode.c // "

int inode__init_inode_block(){
    //reserve the 2nd block for inodes
    bitmap_put(get_blocks_bitmap(), 1, 1);
    return 0;
}

neat_inode_t *inode__get_inode(int inode_i){
    if (inode_i < 0){
        printf ("%sERROR: trying to get inode from index %d!\n", INODE_FILE_NAME, inode_i);
        return NULL;
    }
    neat_inode_t *inode = (neat_inode_t *)(inode__get_inode_base() + inode_i * sizeof(neat_inode_t));
    
    return inode;
}

int inode__get_inode_count(){
    return block_data_size() / sizeof(neat_dir_t) * BLOCK_COUNT; //should round down
}

//should always be blocks_get_block(1) if the
//first block is for bitmaps (and all are meant to fit in there)
void *inode__get_inode_base(){
    return blocks_get_block(1);
}

neat_inode_t *inode__alloc_inode(){
    void *inbm = get_inode_bitmap();

    for (int inode_i = 0; inode_i < inode__get_inode_count(); inode_i++){
        if (!bitmap_get(inbm, inode_i)){
            bitmap_put(inbm, inode_i, 1);
            neat_inode_t *inode = inode__get_inode(inode_i);

            inode->size = 0;
            inode->inode_i = inode_i;
            inode->block_i = alloc_block();
            inode->mode = 040755;//R_OK ^ W_OK ^ X_OK ^ F_OK;
            inode->ctime = time(0);
            inode->mtime = time(0);
            inode->atime = time(0);

            return inode__get_inode(inode_i);
        }
    }
    printf("%sERROR: failed to allocate an inode\n", INODE_FILE_NAME);
    return NULL;
}


int inode__free_inode(int inode_i){
    neat_inode_t *inode = inode__get_inode(inode_i);
    int curr_block_i = inode->block_i;
    int prev_block_i = inode->block_i;

    inode__shrink_inode(inode, 0);

    //while the curr block is valid and it is still in use
    while (curr_block_i > 0 && bitmap_get(get_blocks_bitmap(), curr_block_i)){
        prev_block_i = curr_block_i;
        curr_block_i = get_next_block_i(curr_block_i);
        free_block(prev_block_i);
    }

    void *inbm = get_inode_bitmap();
    bitmap_put(inbm, inode_i, 0);

    
    return 0;
}

int inode__grow_inode(neat_inode_t *inode, int size){
    //increase inode size, and make sure to allocate next blocks if we need them
    
    //if size is the same, nothing to grow!
    if (inode->size == size){
        return 0;
    }
 
    //if size was actually smaller, go shrink
    if (inode->size > size){
        return inode__shrink_inode(inode, size);
    }

    int remainder_on_last_block = inode->size % block_data_size();
    int size_diff = size - inode->size;
    int remaining = size_diff + remainder_on_last_block;

    int curr_last_ext_block_i = get_last_ext_block_i(inode->block_i);

    while (remaining > block_data_size()){
        remaining -= block_data_size();
        curr_last_ext_block_i = allocate_extension_block(curr_last_ext_block_i);
    }
    
    inode->size = size;
    return 0;
}

int inode__shrink_inode(neat_inode_t *inode, int size){
    //decrease inode size, and make sure to free all extension blocks that are no longer used
    
    //if size is the same, nothing to shrink!
    if (inode->size == size){
        return 0;
    }
 
    //if size was actually larger, go grow
    if (inode->size < size){
        return inode__grow_inode(inode, size);
    }

    int remainder_on_last_block = inode->size % block_data_size();
    int size_diff = inode->size - size;
    int remaining = size_diff - remainder_on_last_block;

    int curr_last_ext_block_i = get_last_ext_block_i(inode->block_i);

    while (remaining > 0){
        remaining -= block_data_size();
        free_block(curr_last_ext_block_i);
        //not optimized but take care of later
        curr_last_ext_block_i = get_last_ext_block_i(inode->block_i);
    }

    inode->size = size;
    return 0;
}