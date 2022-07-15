#ifndef NEAT_DIRECTORY_H
#define NEAT_DIRECTORY_H

#include "blocks.h"
#include "neat_inode.h"

#define NEAT_DIR_NAME_LENGTH 48

typedef struct neat_dir {
    char name[NEAT_DIR_NAME_LENGTH];
    int inode_i;
} neat_dir_t;

//Initialize the root inode.
//Returns 0 on success, -1 on failure
void dir__init_root();

//Gets an inode index from an inode [dd] based on the [name] of the directory
//Returns the inode index on success, -1 on failure
int dir__inode_i_from_inode(neat_inode_t *dd, const char *name);

//Gets an inode index from a [path]
//Returns the inode index on success, -1 on failure
int dir__inode_i_from_path(const char *path);

//Adds a director to an inode [dd] with a the directory [name] and with the inode index [inum] it belongs to
//Returns 0 on success, -1 on failure
int dir__add_dir_to_inode(neat_inode_t *dd, const char *name, int inum);

//Removes a directory [name] from an inode [dd]
//Returns 0 on success, -1 on failure
int dir__rm_dir_from_inode(neat_inode_t *dd, const char *name);

//Populates the [parent_path] and [child_name] strings by seperating the path into each respectively
//Returns 0 on success, -1 on failure
int dir__parent_child_from_path(const char *path, char *parent_path, char *child_name);

//Prints a standardized error for when trying to get an inode index from a path
void dir__print_error__inode_i_from_path(char *file_name, char *inode_name, const char *path);
#endif