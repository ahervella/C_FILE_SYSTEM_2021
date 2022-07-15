# C File System - 2021
This was my final assignment for my Computer Systems class in which we were tasked with creating a filesystem in C using the FUSE filesystem driver! Below is the original assignment instructions. I was very happy with my results, as I recieved an A for the assignment!



_____________________

# Original Assignment Instructions
In this assignment you will build a [FUSE](https://en.wikipedia.org/wiki/Filesystem_in_Userspace) filesystem driver that will let you
mount a 1MB disk image (data file) as a filesystem. The implementation only needs to support files of size <= 4K.

## Step 1: Install FUSE

For this assignment you will need to use your local Ubuntu VM (or another local
modern Linux). You'll need to install the following packages:
- `libfuse-dev`
- `libbsd-dev`
- `pkg-config`

Running
```
$ sudo apt-get install libfuse-dev libbsd-dev pkg-config
```
should do the trick.

Make sure your working directory is a proper Linux filesystem, not a remote-mounted Windows or Mac directory, i.e., do not use `/vagrant`.

## Step 2: Implement a basic filesystem

You should extend the provided starter code so that it lets you do the following:

 - Create files.
 - List the files in the filesystem root directory (where you mounted it).
 - Write to small files (under 4k).
 - Read from small files (under 4k).
 - Rename files. 
 - OPTIONAL: Delete files.

You will need to extend the functionality in [`nufs.c`](nufs.c), which only provides a simulated filesystem to begin with. This will require that you come up with a structure for how the file system will store data in it's 1MB "disk". See the [filesystem slides](https://course.ccs.neu.edu/cs3650f21/new/Lectures/12/lecture_16--File_systems.pdf) and [OSTEP, Chapter 40](https://pages.cs.wisc.edu/~remzi/OSTEP/file-implementation.pdf) for inspiration.

We have provided some helper code in the [`helpers/`](helpers) directory. You can use it if you want, but you don't have to. However, `blocks.{c,h}` and `bitmap.{c,h}` might save you some time as these implement block manipulation over a binary disk image. Feel free to extend the functionality if needed.

Some additional headers that might be useful are provided in the `hints` directory. These are just some data definitions and function prototypes to serve as an inspiration. Again, feel free to implement your own structs and abstractions.

## Deliverable

1. Submit your Github repo to Gradescope under Assignment 6.
2. DO NOT submit any binary files (no executables, no .o files, no disk image files).

## Provided Makefile and Tests

The provided [Makefile](Makefile) should simplify your development cycle. It provides the following targets:

- `make nufs` - compile the `nufs` binary. This binary can be run manually as follows:
  
  ```
  $ ./nufs [FUSE_OPTIONS] mount_point disk_image
  ```
- `make mount` - mount a filesystem (using `data.nufs` as the image) under `mnt/` in the current directory
- `make unmount` - unmount the filesystem
- `make test` - run some tests on your implementation. This is a subset of tests we will run on your submission. It should give you an idea whether you are on the right path. You can ignore tests for deleting files if you are not implementing that functionality.
- `make gdb` - same as `make mount`, but run the filesystem in GDB for debugging
- `make clean` - remove executables and object files, as well as test logs and the `data.nufs`.
