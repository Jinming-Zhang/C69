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
        printf("invalid commands\n");
        return 0;
    }

    /* extract name of link file and directory of where the link will be created */
    char *lk_name, *lk_dir;

    // extract the name of new directory, the path of the directory
    if(get_dir_filename(lk_path, &lk_dir, &lk_name) != 0){
      printf("error on extracting link name and dir\n");
      return 0;
    }
    
    
    if(is_s_link){
      printf("creating symbolic link under dir: %s with name %s\n", lk_dir, lk_name);
      return 0;
    }
    printf("creating hard link under dir: %s with name %s\n", lk_dir, lk_name);
    /* create link (hard link) */
    //find inode of src file inode for the link directory
    struct ext2_inode *root, *src_inode, *src_dir_inode, *lk_dir_inode;
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

    // update the target directory to the inode of sourse
    int last_blk;
    unsigned char filetype;
    last_blk = lk_dir_inode->i_block[0];
    filetype = EXT2_FT_SYMLINK;
    // get the inode number of src file
    char *src_dir, *src_name;
    if(get_dir_filename(src_path, &src_dir, &src_name) != 0){
      printf("error on extracting src name and dir\n");
      return 0;
    }
    printf("src dir: %s, src filename: %s\n", src_dir, src_name);
    int src_nd_nb;
    struct ext2_dir_entry_2 *src_file_entry;
    src_dir_inode = traverse(src_dir, root, disk);
    src_file_entry = get_entry(src_dir_inode->i_block[0], disk);
    src_file_entry = find_entry(src_name, src_file_entry);
    src_nd_nb = src_file_entry->inode;
    update_entry_block(last_blk, filetype, src_nd_nb, lk_name, disk);
    (src_inode->i_links_count)++; 





    free(src_dir);
    free(src_name);
    free(lk_dir);
    free(lk_name);
    return 0;
}
