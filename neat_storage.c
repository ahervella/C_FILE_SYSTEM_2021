#include "neat_storage.h"
#include "blocks.h"
#include "neat_inode.h"
#include "neat_directory.h"
#include "bitmap.h"

#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#include <errno.h>

#define STORAGE_FILE_NAME "neat_storage.c // "

void storage_init(const char *path){
    blocks_init(path);
    inode__init_inode_block();
    dir__init_root();
}

int storage_stat(const char *path, struct stat *st){
    //get the inode at said path
    int inode_i = dir__inode_i_from_path(path);
    if (inode_i < 0){
        dir__print_error__inode_i_from_path(STORAGE_FILE_NAME, "storage_stat inode", path);
        return -ENOENT;
    }
    neat_inode_t *inode = inode__get_inode(inode_i);
    
    //put all of its stats in the stat pointer
    st->st_ino = inode->inode_i;
    st->st_mode = inode->mode;
    st->st_size = inode->size;
    st->st_uid = getuid();
    
    return 0;
}

int storage_read(const char *path, char *buf, size_t size, off_t offset){

    return storage_get_data(path, NULL, buf, size, offset, 0);
}

int storage_write(const char *path, const char *buf, size_t size, off_t offset){

    return storage_get_data(path, buf, NULL, size, offset, 1);
}

int storage_get_data(const char *path, const char *buf_read_from, char *buf_write_to, size_t size, off_t offset, int readOrWrite){
    //get inode from path
    int inode_i = dir__inode_i_from_path(path);
    if (inode_i < 0){
        dir__print_error__inode_i_from_path(STORAGE_FILE_NAME, "storage_get_data inode", path);
        -ENOENT;
    }
    neat_inode_t *inode = inode__get_inode(inode_i);

    //so we can adjust the base of the pnts without changing the ones we need to
    int buff_offset = 0;

    int curr_block_i = inode->block_i;

    //save casted as int
    int remaining_size = size;
    int offset_int = offset;
    
    //grow the inode for the new data we are writing to if necessary
    if (readOrWrite == 1){
        int req_size = offset_int + remaining_size;
        if (inode->size < req_size){
            inode__grow_inode(inode, req_size);
        }
    }

    //get data of inode starting from [offset]
    void *block_pntr = blocks_get_block(inode->block_i);
    block_pntr += offset_int;

    while (remaining_size > 0){

        int data_length = remaining_size > block_data_size() ? block_data_size() : remaining_size;

        if (readOrWrite == 0){
            //we are reading data from storage, writing to buf
            //only read the size we want or have available
            data_length = inode->size < data_length ? inode->size : data_length;

            strncpy(buf_write_to + buff_offset, block_pntr, data_length);
        }
        else if (readOrWrite == 1) {
            //we are wriiting data from the buf to the storage
            
            strncpy((char *)block_pntr, buf_read_from + buff_offset, data_length);
        }


        remaining_size -= block_data_size();
        buff_offset += data_length;

        curr_block_i = get_next_block_i(curr_block_i);

        if (remaining_size > 0 && curr_block_i <= 0){
            if (readOrWrite == 0){
                //nothing left to read so the rest of the buffer to write to can remain the same!
                return 0;
            }
            else if (readOrWrite == 1){
                printf("%sERROR: tried to write past the file size for file: %s", STORAGE_FILE_NAME, path);
                return -ENOENT;
            }
        }

        //move to next block in case we still have data in next block on next while iteration
        block_pntr = blocks_get_block(curr_block_i);
    }

    return 0;
}


int storage_truncate(const char *path, off_t size){
    //get inode from path
    int inode_i = dir__inode_i_from_path(path);
    if (inode_i < 0){
        dir__print_error__inode_i_from_path(STORAGE_FILE_NAME, "storage_truncate inode", path);
        return -ENOENT;
    }
    neat_inode_t *inode = inode__get_inode(inode_i);

    //make to designated size
    if (size > inode->size){
        inode__grow_inode(inode, size);
    }
    else if (size < inode->size){
        inode__shrink_inode(inode, size);
    }
                
    //if == size
    return 0;
}

int storage_mknod(const char *path, int mode){
    //make the inode first
    neat_inode_t *new_child_inode = inode__alloc_inode();
    
    new_child_inode->mode = mode;
    new_child_inode->size = 0;

    //the path recieved is the full parent directory + new node path
    //split it to get just the parent, then the child as the name
    char parent_path[strlen(path)];
    char child_name[strlen(path)];

    int rv = dir__parent_child_from_path(path, parent_path, child_name);
    if (rv != 0){
        printf("%sERROR: failed to get the parent and child paths from the full path path %s\n", STORAGE_FILE_NAME, path);
        return -ENOENT;
    }

    int parent_inode_i = dir__inode_i_from_path(parent_path);
    if (parent_inode_i != 0){
        dir__print_error__inode_i_from_path(STORAGE_FILE_NAME, "parent_inode", parent_path);
        return -ENOENT;
    }
    neat_inode_t *parent_inode = inode__get_inode(parent_inode_i);
    

    //now link it in the directory
    rv = dir__add_dir_to_inode(parent_inode, child_name, new_child_inode->inode_i);
    if (rv != 0){
        printf("%sERROR: failed to add the directory to the inode index %d\n", STORAGE_FILE_NAME, parent_inode_i);
        return -ENOENT;
    }

    return 0;
}

int storage_unlink(const char *path){
    //the path given is the full child directory, split it up
    char parent_path[strlen(path)];
    char child_name[strlen(path)];

    int rv = dir__parent_child_from_path(path, parent_path, child_name);
    if (rv != 0){
        printf("%sERROR: failed to get the parent and child paths from the full path path %s\n", STORAGE_FILE_NAME, path);
        return -ENOENT;
    }

    int parent_inode_i = dir__inode_i_from_path(parent_path);
    if (parent_inode_i < 0){
        dir__print_error__inode_i_from_path(STORAGE_FILE_NAME, "parent_inode", parent_path);
        return -ENOENT;
    }
    neat_inode_t *parent_inode = inode__get_inode(parent_inode_i);

    rv = dir__rm_dir_from_inode(parent_inode, child_name);
    if (rv != 0){
        return -ENOENT;
    }
    
    return 0;
}

int storage_link(const char *from, const char *to){
    //from is the full child directory
    //to is the new parent director
    char dummy_parent_path[strlen(from)];
    char child_name[strlen(from)];

    int rv = dir__parent_child_from_path(from, dummy_parent_path, child_name);
    if (rv != 0){
        printf("%sERROR: failed to get the parent and child paths from the full path path %s\n", STORAGE_FILE_NAME, from);
        return -ENOENT;
    }

    int new_parent_inode_i = dir__inode_i_from_path(to);
    if (new_parent_inode_i < 0){
        dir__print_error__inode_i_from_path(STORAGE_FILE_NAME, "new_parent_inode", to);
        return -ENOENT;
    }
    neat_inode_t *new_parent_inode = inode__get_inode(new_parent_inode_i);

    int child_inode_i = dir__inode_i_from_path(from);
    if (child_inode_i < 0){
        dir__print_error__inode_i_from_path(STORAGE_FILE_NAME, "child_inode", from);
        return -ENOENT;
    }

    rv = dir__add_dir_to_inode(new_parent_inode, child_name, child_inode_i);
    if (rv != 0){
        return -ENOENT;
    }
    
    return 0;
}

int storage_rename(const char *from, const char *to){
    //link the new one before unlinking so we can still find
    //it via its old link path
    int rv = storage_link(from, to);
    if (rv != 0){
        return -ENOENT;
    }

    rv = storage_unlink(from);
    if (rv != 0){
        return -ENOENT;
    }

    return 0;
}

int storage_set_time(const char *path, const struct timespec ts[2]){
    int inode_i = dir__inode_i_from_path(path);
    if (inode_i < 0){
        dir__print_error__inode_i_from_path(STORAGE_FILE_NAME, "storage_set_time inode", path);
        return -ENOENT;
    }
    neat_inode_t *inode = inode__get_inode(inode_i);

    inode->atime = ts[0].tv_sec;
    inode->mtime = ts[1].tv_sec;

    return 0;
}