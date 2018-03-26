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

#define ARGUMENTS 4

unsigned char *disk;

/* This program takes three command line arguments.
   	The first is the name of an ext2 formatted virtual disk.
   	The second is the path to a file on your native operating system
   	The third is an absolute path on your ext2 formatted disk.
   The program should work like cp, copying the file on your native file system
   onto the specified location on the disk. If the specified file or target
   location does not exist, then your program should return the appropriate
   error (ENOENT). Please read the specifications of ext2 carefully, some things
   you will not need to worry about (like permissions, gid, uid, etc.), while
   setting other information in the inodes may be important (e.g., i_dtime).  
*/

int main(int argc, char **argv) {
    struct ext2_inode *root, *file, *destiny;
	char *file_path, *cp_path;
	int i,j;

    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
	    perror("mmap");
	    exit(1);
    }
    
    /* validate and parse commands */
    // disk.img path
    if(argc == ARGUMENTS){
        file_path = malloc(sizeof(char) * strlen(argv[2]));
        cp_path = malloc(sizeof(char) * strlen(argv[3]));
        strcpy(file_path, argv[2]);
        strcpy(cp_path, argv[3]);
    }
    else{
        printf("invalid commands\n");
        return 0;
    } 
   
    /*printing each directory on a separate line.*/
    root = get_inode(ROOT, disk);
	// get the first path in after the root node
	//printf("type of root %c\n", check_inode_type(root));
	file = traverse(file_path, root, disk);
	destiny = traverse(cp_path, root, disk);
	printf("\n\nFinal result by ext2_ls-----\n");
	if(file == NULL && destiny == NULL){
		fprintf(stderr, "No such file or directory\n");
		return ENOENT;
	}
	else{
		// check available resources
		int blk_bitmap[BLOCKS], ind_bitmap[INODES];
		get_bitmap(blk_bitmap, disk, BLOCK);
		get_bitmap(ind_bitmap, disk, INDOE);
		/* to copy a file */
		// get the 
		char *content = (void *) get_block(file->i_block[0], disk);
		int free_blk = free_position(blk_bitmap, BLOCK);
		
		for(i=0; i<(file->i_blocks)/2; i++){

		}
	}
	// else{
	// 	// if it is a file, just print its name
	// 	type = check_inode_type(last);
	// 	if(type == 'f'){
	// 		char *name;
	// 		name = strrchr(abs_path, '/') + 1;
	// 		printf("%s\n", name);
	// 	}else if(type == 'd'){
	// 		struct ext2_dir_entry_2 *entry = get_entry(last->i_block[0], disk);
	// 		int block_count, cur_size, dir_size;
	// 		char name[EXT2_NAME_LEN + 1];
	// 		block_count = 1;
	// 		cur_size = 0;
	// 		dir_size = last->i_size;
	// 		// print first two directory according to command
			
	// 		while(cur_size < EXT2_BLOCK_SIZE){
	// 			memcpy(name, entry->name, entry->name_len);
 //        		name[entry->name_len] = 0;
	// 			printf("%s\n", name);
	// 			// look up to next entry, update variables
	// 			entry = (void *) entry + entry->rec_len;
	// 			cur_size = cur_size + entry->rec_len;
	// 			if(cur_size > EXT2_BLOCK_SIZE && 
	// 					(cur_size + block_count * EXT2_BLOCK_SIZE)< dir_size){
	// 				// reset cur_size, entry points to next block, block_count++
	// 				cur_size = 0;
	// 				block_count++;
	// 				entry = get_entry(last->i_block[0]+block_count, disk);
	// 			}	
	// 		}
	// 	}
	//}

			
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
    return 0;
}
