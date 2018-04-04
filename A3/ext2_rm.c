#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"
#include "helps.h"
#include <string.h>



unsigned char *disk;

/* This program takes two command line arguments.
	The first is the name of an ext2 formatted virtual disk
	The second is an absolute path to a file or link (not a directory) on that disk.
   The program should work like rm, removing the specified file from the disk.
   If the file does not exist or if it is a directory, then your program should
   return the appropriate error. Once again, please read the specifications of
   ext2 carefully, to figure out what needs to actually happen when a file or
   link is removed (e.g., no need to zero out data blocks, must set i_dtime in
   the inode, removing a directory entry need not shift the directory entries
   after the one being deleted, etc.).
	
   Bonus(5% extra): Implement an additional "-r" flag (after the disk image
	argument), which allows removing directories as well. In this case, you will
	have to recursively remove all the contents of the directory specified in
	the last argument. If "-r" is used with a regular file or link, then it
	should be ignored (the ext2_rm operation should be carried out as if the
	flag had not been entered). If you decide to do the bonus, make sure first
	that your ext2_rm works, then create a new copy of it and rename it to
	ext2_rm_bonus.c, and implement the additional functionality in this separate
	source file. 
*/
int main(int argc, char **argv) {
    char *rm_path;
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
	    perror("mmap");
	    exit(1);
    }
    
    /* validate and parse commands */
    // disk.img path
    if(argc == 3){
      rm_path = argv[2];
    }else{
        printf("Invalid arguments\n");
        return 0;
    }

    /* remove the file */

    // extract the filename and its directory
    char *rm_dir, *rm_filename;
    if(get_dir_filename(rm_path, &rm_dir, &rm_filename) != 0){
      fprintf(stderr, "Error on extract filename and directory\n");
      return 0;
    }
    // get the inode representing the dir and the file
    struct ext2_inode *root, *dir_inode;// *file_inode;
    root = get_inode(EXT2_ROOT_INO, disk);
    dir_inode = traverse(rm_dir, root, disk);
    if(dir_inode == NULL){
      fprintf(stderr, "file or directory doesn't exist\n");
      return ENOENT;
    }

    // update dir entry
    if(delete_entry_block(dir_inode->i_block[0], rm_filename, disk) != 0){
      fprintf(stderr, "File doesn't exist\n");
      return ENOENT;
    }

   return 0;
}
