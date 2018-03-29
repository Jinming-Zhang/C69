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

/* This program takes two command line arguments. The first is the name of an
   ext2 formatted virtual disk. The second is an absolute path on the
   ext2 formatted disk. The program should work like ls -1 
   (that's number one "1", not lowercase letter "L"), printing each directory
   entry on a separate line. If the flag "-a" is specified
   (after the disk image argument), your program should also print the . and ..
   entries. In other words, it will print one line for every directory entry in
   the directory specified by the absolute path. If the path does not exist,
   print "No such file or directory", and return an ENOENT. Directories passed
   as the second argument may end in a "/" - in such cases the contents of the
   last directory in the path (before the "/") should be printed (as ls would do).     
   Additionally, the path (the last argument) may be a file or link.
   In this case, your program should simply print the file/link name
   (if it exists) on a single line, and refrain from printing the . and ... 
*/
int main(int argc, char **argv) {
    struct ext2_inode *root, *last;
    int print_par_dir;
    int i;
	char *abs_path;
	char type;
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
	    perror("mmap");
	    exit(1);
    }
    
    /* validate and parse commands */
    // disk.img path
    if(argc == 3){
        abs_path = argv[2];
        print_par_dir = FALSE;
    }
    // disk.img -a path
    else if(strcmp(argv[2], "-a") == 0 && argc == 4){
        abs_path = argv[3];
        print_par_dir = TRUE;
    }
    else{
        printf("invalid commands\n");
        return 0;
    } 
    
    /*printing each directory on a separate line.
    int bitma[BLOCKS];
    int indmp[INODES];
    printf("original bitmaps:\n");
    get_bitmap(bitma, disk, BLOCK_BITMAP);
    get_bitmap(indmp, disk, INODE_BITMAP);
	print_bitmap(bitma, BLOCKS);
	print_bitmap(indmp, INODES);   
	// set the 111 bit in block bit map to 1
    set_bitmap(bitma, disk, BLOCK_BITMAP, 111, FALSE);
    set_bitmap(indmp, disk, INODE_BITMAP, 22,FALSE);
    printf("value of copied bitmap array:\n");
    print_bitmap(bitma, BLOCKS);
    print_bitmap(indmp, INODES);
    printf("values in the img bitmap blocks:\n");
    get_bitmap(bitma, disk, BLOCK_BITMAP);
    get_bitmap(indmp, disk, INODE_BITMAP);
    print_bitmap(bitma, BLOCKS);
    print_bitmap(indmp, INODES);  
    printf("size: %d, linkcount: %d blocks: %d\n", root->i_size,
    root->i_links_count, root->i_blocks);
      */
   
    /*printing each directory on a separate line.*/
    root = get_inode(EXT2_ROOT_INO, disk);
	// get the first path in after the root node
	//printf("type of root %c\n", check_inode_type(root));
	last = traverse(abs_path, root, disk);
	
	//printf("\n\nFinal result by ext2_ls-----\n");
	if(last == NULL){
		fprintf(stderr, "No such file or directory\n");
		return ENOENT;
	}else{
		// if it is a file, just print its name
		type = check_inode_type(last);
		if(type == 'f'){
			char *name;
			name = strrchr(abs_path, '/') + 1;
			printf("%s\n", name);
		}else {//(type == 'd'){
			struct ext2_dir_entry_2 *entry = get_entry(last->i_block[0], disk);
			int block_count, cur_size, dir_size;
			char name[EXT2_NAME_LEN + 1];
			block_count = 1;
			cur_size = 0;
			dir_size = last->i_size;
			// skip first two directory according to command
			if (print_par_dir == FALSE){
				for(i=0; i<2; i++){
					entry = (void *) entry + entry->rec_len;
					cur_size = cur_size + entry->rec_len;
				}
			}
			while(cur_size < EXT2_BLOCK_SIZE){
				memcpy(name, entry->name, entry->name_len);
        		name[entry->name_len] = 0;
				printf("%s\n", name);
				// look up to next entry, update variables
				entry = (void *) entry + entry->rec_len;
				cur_size = cur_size + entry->rec_len;
				if(cur_size > EXT2_BLOCK_SIZE && 
						(cur_size + block_count * EXT2_BLOCK_SIZE)< dir_size){
					// reset cur_size, entry points to next block, block_count++
					cur_size = 0;
					block_count++;
					entry = get_entry(last->i_block[0]+block_count, disk);
				}
			}
		}
	}
   return 0;
}
