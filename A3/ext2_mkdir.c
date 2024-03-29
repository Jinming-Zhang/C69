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
   	The first is the name of an ext2 formatted virtual disk.
   	The second is an absolute path on your ext2 formatted disk.
   The program should work like mkdir, creating the final directory on the
   specified path on the disk. If any component on the path to the location
   where the final directory is to be created does not exist or if the specified
   directory already exists, then your program should return the appropriate
   error (ENOENT or EEXIST). Again, please read the specifications to make sure
   you're implementing everything correctly (e.g., directory entries should be
   aligned to 4B, entry names are not null-terminated, etc.). 
*/
int main(int argc, char **argv) {
    
    char *dir_path, *dir_name, *par_dir_path;

    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
	    perror("mmap");
	    exit(1);
    }
    
    /* validate and parse commands */
    // disk.img path
    if(argc == 3){
       dir_path = argv[2];
    }else{
        fprintf(stderr, "Invalid arguments\n");
        return 0;
    }

    /* create a new directory under the given path */
    // extract the name of new directory, the path of the directory
    if(get_dir_filename(dir_path, &par_dir_path, &dir_name) != 0){
      fprintf(stderr, "Error on extracting directory and file name\n");
      return 0;
    }

    /* make the new directory in the disk */
    struct ext2_inode *par_inode, *root;

    root = get_inode(EXT2_ROOT_INO, disk);
    par_inode = traverse(par_dir_path, root, disk);
    // check if the directory already exist
    if(find_file_inode(par_inode, dir_name, disk) != NULL){
      fprintf(stderr, "Directory already exist\n");
      return EEXIST;
    }else{
      
      // allocate necessary resources
      int blk_bitmap[BLOCKS], ind_bitmap[INODES], free_ind, free_blk;
      get_bitmap(blk_bitmap, disk, BLOCKS);
      get_bitmap(ind_bitmap, disk, INODES);

      free_ind = free_position(ind_bitmap, INODES);
      // to initialize i_block[0]
      free_blk = free_position(blk_bitmap, BLOCKS);
      set_bitmap(blk_bitmap, disk, BLOCKS, free_blk, USING);
      set_bitmap(blk_bitmap, disk, INODES, free_ind, USING);
      
      // get the free inode and initialize it to represent this new directory
      struct ext2_inode *new_dir_inode;

      new_dir_inode = get_inode(free_ind, disk);

      new_dir_inode->i_mode = EXT2_S_IFDIR;
      new_dir_inode->i_links_count = 1;
      new_dir_inode->i_blocks = EXT2_BLOCK_SIZE/DISK_SECTOR;
      new_dir_inode->i_block[0] = free_blk;
      new_dir_inode->i_size = EXT2_BLOCK_SIZE;
      // update entry for directory itself
      struct ext2_dir_entry_2 *entry;
      entry = get_entry(free_blk, disk);
      entry->inode = free_ind;
      entry->name_len = strlen(".");
      memcpy(entry->name, ".", strlen("."));
      entry->file_type = EXT2_FT_DIR;
      entry->rec_len = 12;

      // update entry for the parent directory
      entry = (void *) entry + entry->rec_len;
      if(strcmp(par_dir_path,"/") == 0){
        entry->inode = EXT2_ROOT_INO;
      }else{
        char *pp_dir, *pp_name;
        if(get_dir_filename(par_dir_path, &pp_dir, &pp_name) != 0){
          fprintf(stderr, "Error on extracting directory and file name\n");
          return 0;
        }
        struct ext2_dir_entry_2 *ppent;
        ppent= find_entry(pp_name, 
                  get_entry((traverse(pp_dir, root, disk)->i_block[0]), disk));
        entry->inode = ppent->inode;
      }

      entry->name_len = strlen("..");
      memcpy(entry->name, "..", strlen(".."));
      entry->file_type = EXT2_FT_DIR;
      entry->rec_len = 1012;
      
      /* update the entries of the parent directory */
      // compute the total blocks occupied by the par_dir
      int last_dir_blk, total_blk;
      total_blk = total_blks(par_inode->i_blocks, disk);
      if(total_blk <= 12){
        last_dir_blk = par_inode->i_block[total_blk-1];
      }else{
        // check for single indirect blocks
        last_dir_blk = single_indirblk_num(par_inode->i_block[12], total_blk-12, disk);
      }
      update_entry_block(last_dir_blk, EXT2_FT_DIR, free_ind, dir_name, disk);

    }
    
    free(par_dir_path);
    free(dir_name);
    return 0;
}
