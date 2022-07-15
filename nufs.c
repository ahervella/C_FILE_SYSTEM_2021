// based on cs3650 starter code

#include <assert.h>
#include <bsd/string.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FUSE_USE_VERSION 26
#include <fuse.h>

#include "neat_storage.h"
#include "neat_directory.h"
#include "neat_inode.h"

#define NUFS_FILE_NAME "nufs.c // "

// implementation for: man 2 access
// Checks if a file exists.
int nufs_access(const char *path, int mask) {
  //For mask: can accept (from man page): "F_OK, or a mask consisting of the
  //bitwise OR of one or more of R_OK, W_OK, and X_OK"

  //F_OK = tests for the existence of the file.
  //R_OK = if the file exists and grants read access
  //W_OK = if the file exists and grants write access
  //X_OK = if the file exists and grants execute permissions

  int rv = 0;
  
  int inode_i = dir__inode_i_from_path(path);
  if (inode_i < 0){
    dir__print_error__inode_i_from_path(NUFS_FILE_NAME, "nufs_access inode", path);
    rv = -ENOENT;
  }

  else if (mask != F_OK){

    printf("access-> got here...");
    //bitwise operator and
    neat_inode_t *inode = inode__get_inode(inode_i);
    rv = inode->mode & mask == mask ? rv : -EACCES;
  }
  printf("access(%s, %04o) -> %d\n", path, mask, rv);
  return rv;
}

// Gets an object's attributes (type, permissions, size, etc).
// Implementation for: man 2 stat
// This is a crucial function.
int nufs_getattr(const char *path, struct stat *st) {

  if (strcmp(path, "/hello.txt") == 0) {
    st->st_mode = 0100644; // regular file
    st->st_size = 6;
    st->st_uid = getuid();
    return 0;
  }

  int rv = storage_stat(path, st);
  printf("getattr(%s) -> (%d) {mode: %04o, size: %ld}\n", path, rv, st->st_mode,
         st->st_size);
  return rv;
}

// implementation for: man 2 readdir
// lists the contents of a directory
int nufs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                 off_t offset, struct fuse_file_info *fi) {
  
  //doesn't look like I need to worry about the offset and fi...
  
  int rv = 0;
  struct stat st;
  rv = storage_stat("/", &st);
  if (rv != 0){
    printf("Error in nufs readdir with storage_stat...\n");
    rv = -ENOENT;
  }
  filler(buf, ".", &st, 0);
  printf("\nstarted readdir...\n\n");
  int inode_i = dir__inode_i_from_path(path);
  if (inode_i < 0){
    dir__print_error__inode_i_from_path(NUFS_FILE_NAME, "nufs_readdir inode", path);
    rv = -ENOENT;
  }
  neat_inode_t *inode = inode__get_inode(inode_i);

  int dir_content_count = inode->size / sizeof(neat_dir_t);

  for(int i = 0; i < dir_content_count; i++){
    neat_dir_t *dir = (neat_dir_t *)blocks_get_block(inode->block_i) + i;
    if (dir == NULL){
      rv = -ENOENT;
      break;
    }

    //all of the next chunk is just to correctly populate the st

    //plus 1 to accomodate for extra '/'
    const int new_full_path_len = strlen(path) + strlen(dir->name) + 1;
    char new_full_path[new_full_path_len];
    //new_full_path = path + / + dir->name
    strcpy(new_full_path, path);
    strcat(new_full_path, "/");
    strcat(new_full_path, dir->name);
    rv = storage_stat(new_full_path, &st);
    if (rv != 0){
      //something went wrong
      printf("Error in nufs readdir with storage_stat...\n");
      rv = -ENOENT;
      break;
    }
    printf("DEBUG_REEADADDR: dir entry name = %s\n", dir->name);
    filler(buf, dir->name, &st, 0);
  }

  
  printf("readdir(%s) -> %d\n", path, rv);
  return rv;
}

// mknod makes a filesystem object like a file or directory
// called for: man 2 open, man 2 link
// Note, for this assignment, you can alternatively implement the create
// function.
int nufs_mknod(const char *path, mode_t mode, dev_t rdev) {
  /*
  int rv = -1;
  printf("mknod(%s, %04o) -> %d\n", path, mode, rv);
  return rv;
  */
  int rv = storage_mknod(path, mode);
  printf("mknod(%s, %04o) -> %d\n", path, mode, rv);
  return rv;
}

// most of the following callbacks implement
// another system call; see section 2 of the manual
int nufs_mkdir(const char *path, mode_t mode) {
  /*
  int rv = nufs_mknod(path, mode | 040000, 0);
  printf("mkdir(%s) -> %d\n", path, rv);
  return rv;
  */

  int rv = nufs_mknod(path, mode, 0);
  printf("mkdir(%s) -> %d\n", path, rv);
  return rv;
}

int nufs_unlink(const char *path) {
  /*
  int rv = -1;
  printf("unlink(%s) -> %d\n", path, rv);
  return rv;
  */

  int rv = storage_unlink(path);
  printf("unlink(%s) -> %d\n", path, rv);
  return rv;
}

int nufs_link(const char *from, const char *to) {
  /*
  int rv = -1;
  printf("link(%s => %s) -> %d\n", from, to, rv);
  return rv;
  */
  int rv = storage_link(from , to);
  printf("link(%s => %s) -> %d\n", from, to, rv);
  return rv;
}

int nufs_rmdir(const char *path) {
  /*
  int rv = -1;
  printf("rmdir(%s) -> %d\n", path, rv);
  return rv;
  */
  
  int inode_i = dir__inode_i_from_path(path);
  if (inode_i < 0){
    dir__print_error__inode_i_from_path(NUFS_FILE_NAME, "nufs_rmdir inode", path);
    return -ENOENT;
  }

  storage_unlink(path);
  inode__free_inode(inode_i);
  return 0;
}

// implements: man 2 rename
// called to move a file within the same filesystem
int nufs_rename(const char *from, const char *to) {
  int rv = storage_rename(from, to);
  printf("rename(%s => %s) -> %d\n", from, to, rv);
  return rv;
}

int nufs_chmod(const char *path, mode_t mode) {
  int rv = 0;

  int inode_i = dir__inode_i_from_path(path);
  if (inode_i < 0){
    dir__print_error__inode_i_from_path(NUFS_FILE_NAME, "nufs_chmod inode", path);
    rv = -ENOENT;
  }
  else{
    neat_inode_t *inode = inode__get_inode(inode_i);
    inode->mode = mode;
  }
  printf("chmod(%s, %04o) -> %d\n", path, mode, rv);
  return rv;
}

int nufs_truncate(const char *path, off_t size) {
  int rv = storage_truncate(path, size);
  printf("truncate(%s, %ld bytes) -> %d\n", path, size, rv);
  return rv;
}

// This is called on open, but doesn't need to do much
// since FUSE doesn't assume you maintain state for
// open files.
// You can just check whether the file is accessible.
int nufs_open(const char *path, struct fuse_file_info *fi) {
  //checks if this is accesible
  int rv = nufs_access(path, F_OK);
  printf("open(%s) -> %d\n", path, rv);
  return rv;
}

// Actually read data
int nufs_read(const char *path, char *buf, size_t size, off_t offset,
              struct fuse_file_info *fi) {
  
  int rv = storage_read(path, buf, size, offset);
  if (rv == 0){
    rv = size;
  }
  printf("read(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
  return rv;
}

// Actually write data
int nufs_write(const char *path, const char *buf, size_t size, off_t offset,
               struct fuse_file_info *fi) {
  
  int rv = storage_write(path, buf, size, offset);
  printf("write(%s, %ld bytes, @+%ld) -> %d\n", path, size, offset, rv);
  return rv;
}

// Update the timestamps on a file or directory.
int nufs_utimens(const char *path, const struct timespec ts[2]) {
  int rv = storage_set_time(path, ts);
  printf("utimens(%s, [%ld, %ld; %ld %ld]) -> %d\n", path, ts[0].tv_sec,
         ts[0].tv_nsec, ts[1].tv_sec, ts[1].tv_nsec, rv);
  return rv;
}

// Extended operations
int nufs_ioctl(const char *path, int cmd, void *arg, struct fuse_file_info *fi,
               unsigned int flags, void *data) {
  int rv = 0;
  printf("ioctl(%s, %d, ...) -> %d\n", path, cmd, rv);
  return rv;

  //what does this even do?
}

void nufs_init_ops(struct fuse_operations *ops) {
  memset(ops, 0, sizeof(struct fuse_operations));
  ops->access = nufs_access;
  ops->getattr = nufs_getattr;
  ops->readdir = nufs_readdir;
  ops->mknod = nufs_mknod;
  // ops->create   = nufs_create; // alternative to mknod
  ops->mkdir = nufs_mkdir;
  ops->link = nufs_link;
  ops->unlink = nufs_unlink;
  ops->rmdir = nufs_rmdir;
  ops->rename = nufs_rename;
  ops->chmod = nufs_chmod;
  ops->truncate = nufs_truncate;
  ops->open = nufs_open;
  ops->read = nufs_read;
  ops->write = nufs_write;
  ops->utimens = nufs_utimens;
  ops->ioctl = nufs_ioctl;
};

struct fuse_operations nufs_ops;

int main(int argc, char *argv[]) {
  assert(argc > 2 && argc < 6);
  storage_init(argv[--argc]);
  nufs_init_ops(&nufs_ops);
  return fuse_main(argc, argv, &nufs_ops, NULL);
}
