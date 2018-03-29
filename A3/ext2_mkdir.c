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
    
    char *dir_path, dir_name[EXT2_NAME_LEN+1], *par_dir_path;

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
    char *temp;
    temp = strrchr(dir_path, '/');
    // cases: /dirname
    //        /dirname/
    //        /path1/dirname
    //        /path1/dirname/
    //        /  (invalid)
    if(dir_path[0] != '/' || strlen(dir_path) == 1){
      fprintf(stderr, "only gives root, no dirname in the argument\n");
      return ENOENT;
    }
    else if(strlen(temp) == 1){// '/' followed by the dir name
      char *pre = temp-1;
      while ((*pre) != '/'){
        pre--;
      }
      int name_len = strlen(pre) - strlen(temp) - 1;
      strncpy(dir_name, pre+1, name_len);
      dir_name[name_len] = '\0';
      pre++; // pre now points to the first letter of the new directory name
      int dir_len = strlen(dir_path) - strlen(pre);
      par_dir_path = malloc(sizeof(char) * dir_len + 1);
      strncpy(par_dir_path, dir_path, dir_len);
      par_dir_path[dir_len] = '\0';
    }else{
      int name_len = strlen(temp) - 1;
      strncpy(dir_name, (temp+1), name_len);
      dir_name[name_len] ='\0';

      temp++;
      int dir_len = strlen(dir_path) - strlen(temp);
      par_dir_path = malloc(sizeof(char) * dir_len + 1);
      strncpy(par_dir_path, dir_path, dir_len);
      par_dir_path[dir_len] = '\0';
    }
    printf("creating dir of name '%s' under directory '%s'\n", dir_name, par_dir_path);



    /* can validate the name of the new directory here */

    /* make the new directory in the disk */
    struct ext2_inode *par_inode, *root;

    root = get_inode(EXT2_ROOT_INO, disk);
    par_inode = traverse(par_dir_path, root, disk);

    // check if the directory already exist
    if(find_file_inode(par_inode, dir_name, disk) != NULL){
      fprintf(stderr, "dir already exist\n");
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

      // get the free inode and initialize it to represent this new directory
      struct ext2_inode *new_dir_inode;

      new_dir_inode = get_inode(free_ind, disk);
      new_dir_inode->i_mode = (new_dir_inode->i_mode) | EXT2_S_IFDIR;
      new_dir_inode->i_links_count = 1;
      new_dir_inode->i_blocks = 1;
      new_dir_inode->i_block[0] = free_blk;

      // update the bitmap and blocks taken by this new directory
      set_bitmap(ind_bitmap, disk, INODE_BITMAP, free_ind, USING);
      set_bitmap(blk_bitmap, disk, BLOCK_BITMAP, free_blk, USING);

      // update the entries of the parent directory
      unsigned char file_type = EXT2_FT_DIR;

      /* need for further compute the total blocks occupied by the par_dir */
      int last_dir_blk;
      last_dir_blk = par_inode->i_block[0];
      printf("updating entry with: blk: %d, type: %c, inode: %d, name: %s\n",
            last_dir_blk, file_type, free_ind, dir_name);
      update_entry_block(last_dir_blk, file_type, free_ind, dir_name, disk);

    }
   return 0;
}
