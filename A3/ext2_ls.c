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
   tar_dir directory in the path (before the "/") should be printed (as ls would do).     
   Additionally, the path (the tar_dir argument) may be a file or link.
   In this case, your program should simply print the file/link name
   (if it exists) on a single line, and refrain from printing the . and ... 
*/
int main(int argc, char **argv) {
    char type, *abs_path;
    struct ext2_inode *root, *tar_dir;
    int print_par_dir;
    int i;

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
        fprintf(stderr, "Invalid arguments\n");
        return 0;
    } 

    /*printing each directory on a separate line.*/
    root = get_inode(EXT2_ROOT_INO, disk);
	// get the first path in after the root node
	tar_dir = traverse(abs_path, root, disk);
	
	if(tar_dir == NULL){
		fprintf(stderr, "No such file or directory\n");
		return ENOENT;
	}else{
		// if it is a file/link, just print its name
		type = check_inode_type(tar_dir);

		if(type == 'f' || type == 'l'){
			char *name;
			name = strrchr(abs_path, '/') + 1;
			printf("%s\n", name);
		}else{
			struct ext2_dir_entry_2 *entry;
			entry = get_entry(tar_dir->i_block[0], disk);
			int block_count, cur_size, total_blk;
			char name[EXT2_NAME_LEN + 1];

			block_count = 1;
			cur_size = 0;
			total_blk = total_blks(tar_dir->i_blocks, disk);
			// skip first two directory according to command
			if (print_par_dir == FALSE){
				for(i=0; i<2; i++){
					cur_size = cur_size + entry->rec_len;
					entry = (void *) entry + entry->rec_len;
				}
			}

			while((cur_size + entry->rec_len) <= EXT2_BLOCK_SIZE){
				// print the entry name
				memcpy(name, entry->name, entry->name_len);
        		name[entry->name_len] = 0;
				printf("%s\n", name);

				// look up to next entry, update variables
				cur_size = cur_size + entry->rec_len;
				entry = (void *) entry + entry->rec_len;
				// if reaches the end of this block
				if(cur_size == EXT2_BLOCK_SIZE){
					// finished the last block of this dir
					if(block_count == total_blk){
						return 0;
					}
					else{
						cur_size = 0;
						block_count++;
						// check the next direct block for this dir
						if(block_count <= 12){
							entry = get_entry(tar_dir->i_block[block_count],
											  disk);
						}else{
							// check for single indirect blocks
							int next_blk;
							next_blk = single_indirblk_num(tar_dir->i_block[12],
														 block_count-12, disk);
         					entry = get_entry(next_blk, disk);
						}
					}	
				}
			}
		}
	}
   return 0;
}
