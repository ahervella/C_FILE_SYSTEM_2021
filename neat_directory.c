#include "neat_directory.h"
#include "neat_inode.h"
#include <string.h>
#include "bitmap.h"

#define ERROR_MSG_INODE_I_FROM_PATH "%sERROR: failed to get %s index from path %s\n"

#define DIR_FILE_NAME "neat_directory.c // "

void dir__init_root(){
    printf("\n\n%sattempting to initialize root...\n", DIR_FILE_NAME);
    //if root node already previously initialized in previous senssion,
    //don't make it again!
    if (bitmap_get(get_inode_bitmap(), 0) == 1){
        printf("\n\n%sroot already initialized...\n", DIR_FILE_NAME);
        return;
    }
    //allocate the first inode and name it properly
    neat_inode_t *inode = inode__alloc_inode();
    if (inode->inode_i != 0 | inode->block_i != 2 ){
        //something went wrong!
        printf("%sERROR: tried to allocate root node, but was not index 0!\n", DIR_FILE_NAME);
    }

    /*
    neat_dir_t *dir_pntr = (neat_dir_t *)blocks_get_block(inode->block_i);
    dir_pntr->inode_i = inode->inode_i;
    strcpy(dir_pntr->name, "/");
    dir_pntr->name[1] = '\0';*/
}

int dir__inode_i_from_inode(neat_inode_t *dd, const char *name){
    //when looking for a directory name, a directory can also be a file
    //which points to an inode and then the data block
    //which is why we can assume all things in this diretory will also be a directory
    neat_dir_t *data_pntr = (neat_dir_t *)blocks_get_block(dd->block_i);
    int num_of_dir = dd->size / sizeof(neat_dir_t);

    for (int i = 0; i < num_of_dir; i++){
        neat_dir_t dir = data_pntr[i];
        if (strcmp(dir.name, name) == 0){
            //found the directory!
            return dir.inode_i;
        }
    }
    //couldn't find it :(
    return -1;
}

int dir__inode_i_from_path(const char *path){
    //root node will always be inode index 0
    int curr_inode_i = 0;
    neat_inode_t *curr_node = inode__get_inode(0);
    
    //if we just requested the root, return the root!
    if (strcmp(path, "/") == 0){
        return curr_inode_i;
    }

    printf("got here in dir__inode_i_from_path... \n");

    char *folder_token = strtok(path, "/");
    while (folder_token != NULL){
        curr_inode_i = dir__inode_i_from_inode(curr_node, folder_token);
        curr_node = inode__get_inode(curr_inode_i);
        if (curr_inode_i < 0){
            //path failed here!
            printf("%sERROR: dir_inode_from_path failed to return an inode!\n", DIR_FILE_NAME);
            return -1;
        }

        //gets the next folder string
        folder_token = strtok(NULL, " ");
    }

    //done and we didn't fail, so this is it!
    return curr_inode_i;
}

int dir__add_dir_to_inode(neat_inode_t *dd, const char *name, int inum){
    
    //currently points to the end of the last neat_dir_t
    neat_dir_t *new_dir_pntr = (neat_dir_t *)(blocks_get_block(dd->block_i) + dd->size);
    neat_dir_t new_dir;
    //grow by one neat_dir_t, and now the pointer new_dir_pntr is pointing to the start of the newly allocated neat_dir_t
    inode__grow_inode(dd, dd->size + sizeof(neat_dir_t));

    //now that we have grown, get the last block pntr start which is the new directory
    new_dir.inode_i = inum;
    strcpy(new_dir.name, name);
    new_dir.name[strlen(name)] = '\0';
    *new_dir_pntr = new_dir;

    return 0;
}

int dir__rm_dir_from_inode(neat_inode_t *dd, const char *name){
    neat_dir_t *dir_pntr = (neat_dir_t *)(blocks_get_block(dd->block_i));
    int num_of_dir = dd->size / sizeof(neat_dir_t);

    int dir_entry_index = -1;

    for (int i = 0; i < num_of_dir; i++){
        neat_dir_t dir = dir_pntr[i];
        if (strcmp(dir.name, name) == 0){
            dir_entry_index = i;
            break;
        }
    }

    if (dir_entry_index == -1){
        //couldn't find the directory in this i_node :(
        return -1;
    }

    for (int i = dir_entry_index; i < num_of_dir - 1; i++){
        
        //now that we are past the dir index we want to remove, make
        //the previous pntr point have the data from the one after it
        //neat_dir_t *curr_dir_prev = dir_pntr + (i - 1);// * sizeof(neat_dir_t);
        neat_dir_t new_dir;

        new_dir.inode_i = dir_pntr[i+1].inode_i;
        strcpy(new_dir.name, dir_pntr[i+1].name);
        new_dir.name[strlen(dir_pntr[i+1].name)] = '\0';

        *(dir_pntr + (i - 1)) = new_dir;
    }
    
    inode__shrink_inode(dd, dd->size - sizeof(neat_dir_t));
}

int dir__parent_child_from_path(const char *path, char *parent_path, char *child_name){
    //find the last '/' and split from there (so iterate backwards) and skip first char incase it
    //ends in '.../../'

    int last_slash_index = -1;

    for (int i = strlen(path) - 2; i >= 0; i--){
        if (path[i] == '/'){
            //found it!
            last_slash_index = i;
            break;
        }
    }

    if (last_slash_index == -1){
        //something went wrong
        printf("%sERROR: something went wrong with getting the parent and child path strings!\n", DIR_FILE_NAME);
        return -1;
    }

    int parent_len = last_slash_index + 1;
    int child_name_len = strlen(path) - parent_len;

    // char parent_pntr[strlen(parent_path)];
    // char *child_pntr[strlen(parent_path)];
    strncpy(parent_path, path, parent_len);
    parent_path[parent_len] = '\0';

    char *offset_path = path + parent_len * sizeof(char);
    memcpy(child_name, offset_path, child_name_len);
    child_name[child_name_len] = '\0';

    // parent_path = parent_pntr;
    // child_name = child_pntr;
    return 0;
}

void dir__print_error__inode_i_from_path(char *file_name, char *inode_name, const char *path){
    printf(ERROR_MSG_INODE_I_FROM_PATH, file_name, inode_name, path);
}