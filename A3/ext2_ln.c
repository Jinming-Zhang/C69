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


#define MAX_PATH_LEN 60
unsigned char *disk;

/* This program takes three command line arguments.
	The first is the name of an ext2 formatted virtual disk.
	The other two are absolute paths on your ext2 formatted disk.
   The program should work like ln, creating a link from the first specified
   file to the second specified path.
   If the source file does not exist (ENOENT)
   if the link name already exists (EEXIST)
   if either location refers to a directory (EISDIR)
   Note that this version of ln only works with files. Additionally, this
   command may take a "-s" flag, after the disk image argument. When this flag
   is used, your program must create a symlink instead
   (other arguments remain the same). If in doubt about correct operation of
   links, use the ext2 specs and ask on the discussion board. 

   // ln [flag] existed new_linkto_existed
*/
int main(int argc, char **argv) {
    char *src_path, *lk_path;
    int is_s_link;
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
	    perror("mmap");
	    exit(1);
    }
    
    /* validate and parse commands */
    // disk.img path
    if(argc == 4){
        src_path = argv[2];
        lk_path = argv[3];
        is_s_link = FALSE;
    }
    // disk.img -a path
    else if(strcmp(argv[2], "-s") == 0 && argc == 5){
       src_path = argv[3];
       lk_path = argv[4];
       is_s_link = TRUE;
    }
    else{
        printf("Invalid arguments\n");
        return 0;
    }

    /* validate the path */
    char *lk_name, *lk_dir;
    // extract the name of new directory, the path of the directory
    if(get_dir_filename(lk_path, &lk_dir, &lk_name) != 0){
      fprintf(stderr, "error on extracting link name and dir\n");
      return 0;
    }
    //find inode of src file inode for the link directory
    struct ext2_inode *root, *src_inode, *lk_dir_inode;
    root = get_inode(EXT2_ROOT_INO, disk);
    src_inode = traverse(src_path, root, disk);
    lk_dir_inode = traverse(lk_dir, root, disk);

    // check if both src and lk are valid
    if(src_inode == NULL){
      return ENOENT;
    }
    else if(check_inode_type(src_inode) != 'f'){
      return EISDIR;
    }
    else if(find_file_inode(lk_dir_inode, lk_name, disk ) != NULL){
      return EEXIST;
    }


    /* create link */
    // variables needed for creating a link (update a directory entry)
    int entry_inode;
    unsigned char filetype;
    // update the target directory to the inode of sourse
    int last_blk;
    last_blk = last_dir_blk(lk_dir_inode, disk);

    // creating a symbolic link
    if(is_s_link){
      filetype = EXT2_FT_SYMLINK;

      // get a free inode for new symbolic link file
      int ind_bitmap[INODES], free_ind;
      get_bitmap(ind_bitmap, disk, INODES);
      free_ind = free_position(ind_bitmap, INODES);
      set_bitmap(ind_bitmap, disk, INODES, free_ind, USING);
      // set up the inode
      struct ext2_inode *new_dir_inode;
      new_dir_inode = get_inode(free_ind, disk);

      // check the lengh of the absolute link path
      if(strlen(src_path)<=MAX_PATH_LEN){// store the path inside the inode
        new_dir_inode->i_blocks = 0;
        int str_len, stored;
        char *tar;
        str_len = strlen(src_path);
        stored = 0;
        // store the path inside i_block[] array
        while(stored <= str_len){
          tar =(char *) &(new_dir_inode->i_block[stored/sizeof(unsigned int)])
                          + stored%sizeof(unsigned int);
          memcpy(tar, &(src_path[stored]), sizeof(char));
          stored++;
        }
      }else{// store the path in a separate block
        new_dir_inode->i_blocks = EXT2_BLOCK_SIZE/DISK_SECTOR;
        int blk_bitmap[BLOCKS], free_blk;
        get_bitmap(blk_bitmap, disk, BLOCKS);
        free_blk = free_position(blk_bitmap, BLOCKS);
        // write the path to the block
        char *tar_blk;
        tar_blk = get_block(free_blk, disk);
        memcpy(tar_blk, src_path, strlen(src_path));
        tar_blk[strlen(src_path)] = '\0';
        new_dir_inode->i_block[0] = free_blk;
        set_bitmap(blk_bitmap, disk, BLOCKS, free_blk, USING);
      }

      // other fields of inode
      new_dir_inode->i_mode =  EXT2_S_IFLNK;
      new_dir_inode->i_links_count = 1;
      new_dir_inode->i_size = EXT2_BLOCK_SIZE;
      entry_inode = free_ind;
    }
    // creating a hard link
    else{
      filetype = EXT2_FT_REG_FILE;
      char *src_dir, *src_name;
      struct ext2_inode *src_dir_inode;
      if(get_dir_filename(src_path, &src_dir, &src_name) != 0){
        printf("error on extracting src name and dir\n");
        return 0;
      }
      // get the inode for the src file
      struct ext2_dir_entry_2 *src_file_entry;
      src_dir_inode = traverse(src_dir, root, disk);
      src_file_entry = get_entry(src_dir_inode->i_block[0], disk);
      src_file_entry = find_entry(src_name, src_file_entry);
      entry_inode = src_file_entry->inode;
      (src_inode->i_links_count)++;

      free(src_dir);
      free(src_name);
    }

    // update the directory entry depends on the type of thel ink
    update_entry_block(last_blk, filetype, entry_inode, lk_name, disk);

    free(lk_dir);
    free(lk_name);
    return 0;
}
