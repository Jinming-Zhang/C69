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
        printf("invalid commands\n");
        return 0;
    }

    /* create a new directory under the given path */
    // extract the name of new directory, the path of the directory
    if(get_dir_filename(dir_path, &par_dir_path, &dir_name) != 0){
      fprintf(stderr, "Error on extracting directory and file name\n");
      return 0;
    }
    printf("creating dir of name '%s' under directory '%s'\n", dir_name, par_dir_path);

    /* make the new directory in the disk */
    struct ext2_inode *par_inode, *root;

    root = get_inode(EXT2_ROOT_INO, disk);
    par_inode = traverse(par_dir_path, root, disk);
    // check if the directory already exist
    if(find_file_inode(par_inode, dir_name, disk) != NULL){
      fprintf(stderr, "Directory already exist\n");
      return EEXIST;
    }else{
      printf("dir %s doesn't exist in %s, now creating...\n", dir_name, par_dir_path);
      
      // allocate necessary resources
      int blk_bitmap[BLOCKS], ind_bitmap[INODES];
      int free_ind, free_blk;
      get_bitmap(blk_bitmap, disk, BLOCK_BITMAP);
      get_bitmap(ind_bitmap, disk, INODE_BITMAP);

      free_ind = free_position(ind_bitmap, INODE_BITMAP);
      // to initialize i_block[0]
      free_blk = free_position(blk_bitmap, BLOCK_BITMAP);
      set_bitmap(blk_bitmap, disk, BLOCK_BITMAP, free_blk, USING);
      printf("freeblock at %d\n", free_blk);
      // get the free inode and initialize it to represent this new directory
      struct ext2_inode *new_dir_inode;

      new_dir_inode = get_inode(free_ind, disk);

      new_dir_inode->i_mode = EXT2_S_IFDIR;
      new_dir_inode->i_links_count = 1;
      new_dir_inode->i_blocks = 2;
      new_dir_inode->i_block[0] = free_blk;
      // update entry for directory itself
      struct ext2_dir_entry_2 *entry;
      entry = get_entry(free_blk, disk);
      entry->inode = free_ind;
      entry->name_len = strlen(".");
      memcpy(entry->name, ".", strlen("."));
      entry->file_type = EXT2_FT_DIR;
      entry->rec_len = 12;
      entry = (void *) entry+entry->rec_len;
      // update entry for the parent directory
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
        printf("update inode number of par dir%d\n", ppent->inode);
      }
      entry->name_len = strlen("..");
      memcpy(entry->name, "..", strlen(".."));
      entry->file_type = EXT2_FT_DIR;
      entry->rec_len = 1012;

    
      new_dir_inode->i_size = EXT2_BLOCK_SIZE;
      printf("type of new inode %c\n", check_inode_type(new_dir_inode));
      // update the bitmap and blocks taken by this new directory
      set_bitmap(ind_bitmap, disk, INODE_BITMAP, free_ind, USING);
      set_bitmap(blk_bitmap, disk, BLOCK_BITMAP, free_blk, USING);
      printf("next freeblk is %d\n",free_position(blk_bitmap, BLOCK_BITMAP));
      // update the entries of the parent directory
      //unsigned char file_type = EXT2_FT_DIR;

      /* compute the total blocks occupied by the par_dir */
      int last_dir_blk, total_blk;
      total_blk = total_blks(par_inode->i_blocks, disk);
      printf("total blocks%d ,%d ", total_blk,par_inode->i_blocks);
      if(total_blk <= 12){
        last_dir_blk = par_inode->i_block[total_blk-1];
        printf("last blk is %d, parinode blk is: %d\n",last_dir_blk, par_inode->i_block[0]);
      }else{
        // check for single indirect blocks
        last_dir_blk = single_indirblk_num(par_inode->i_block[12], total_blk-12, disk);
      }
      printf("updating entry with: blk: %d, type: %d, inode: %d, name: %s\n",
            last_dir_blk, (int) EXT2_FT_DIR, free_ind, dir_name);
      update_entry_block(last_dir_blk, EXT2_FT_DIR, free_ind, dir_name, disk);

    }
    free(par_dir_path);
    free(dir_name);
    return 0;
}
