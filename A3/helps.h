#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"

#include <errno.h>
#include <string.h>

#define TRUE 1
#define FALSE 0
#define ROOT 2
#define RESERVED_NODE 11
#define DISK_SECTOR 512
#define BLOCKS 128
#define INODES 32
#define SUPER_BLOCK_SIZE 1024
#define BG_DESCRIPTOR_SIZE 1024
#define BLK_BTMP_SIZE 1024
#define IND_BTMP_SIZE 1024
#define BYTE 8
#define INODE_BITMAP 8888
#define BLOCK_BITMAP 6666

struct ext2_super_block *sb;
struct ext2_group_desc *gd;
struct ext2_inode root;

struct ext2_inode *find_file_inode(struct ext2_inode *,
							char *, unsigned char *);
							
							
struct ext2_super_block *get_super_block(unsigned char *disk){
    return (struct ext2_super_block *) (disk + EXT2_BLOCK_SIZE);
}

struct ext2_group_desc *get_bg_descriptor(unsigned char *disk){
    return (struct ext2_group_desc *) (disk + EXT2_BLOCK_SIZE + SUPER_BLOCK_SIZE);
}

struct ext2_inode *get_inode(int number, unsigned char *disk){
	gd = get_bg_descriptor(disk);
	return (struct ext2_inode *) (disk + gd->bg_inode_table * EXT2_BLOCK_SIZE
									+ sizeof(struct ext2_inode) * (number-1));
}

/* Bit map helper functions */
void get_bitmap(int *mp, unsigned char *disk, int mode){
    char *mp_ptr;
    int len, i, j;
    gd = get_bg_descriptor(disk);
    // set bitmap pointer correspondingly
    if(mode == BLOCK_BITMAP){
    	mp_ptr = (char *) (disk + gd->bg_block_bitmap * EXT2_BLOCK_SIZE);
    	len = BLOCKS;
    }else{
    	mp_ptr = (char *) (disk + gd->bg_inode_bitmap * EXT2_BLOCK_SIZE);
    	len = INODES;
    }
 	// load bitmap to array
    for(i=0; i<(len/BYTE); i++){
        for(j=0; j<BYTE; j++){
            mp[BYTE*i+j] = (*mp_ptr)>>j & (0x1);
        }
        mp_ptr++;
    }
}

void set_bitmap(int *mp, unsigned char *disk, int mode, int number, int value){
	int tar;
	char* mp_ptr;
	// convert block/inode number to the index in the bitmap
	tar = number - 1;
	gd = get_bg_descriptor(disk);
	// set bitmap pointer correspondingly
    if(mode == BLOCK_BITMAP){
    	mp_ptr = (char *) (disk + gd->bg_block_bitmap * EXT2_BLOCK_SIZE);
    }else{
    	mp_ptr = (char *) (disk + gd->bg_inode_bitmap * EXT2_BLOCK_SIZE);
    }
    
    mp_ptr = mp_ptr + (number/BYTE);
    // set the value to true (1)
    if(value == TRUE){
 		// set the block bit map at the block to 1 and update bitmap
    	(*mp_ptr) = (*mp_ptr) | (1 << (number%BYTE-1));
    	mp[tar] = TRUE;
   	}else{
   		(*mp_ptr) = (*mp_ptr) & ~(1 << (number%BYTE-1));
   		mp[tar] = FALSE;
   	}
}

void print_bitmap(int *mp, int len){
    int i;
    for(i=0; i<len; i++){
        printf("%d", mp[i]);
        if(i!=0 && (i+1)%BYTE==0){
            printf(" ");
        }
    }
    printf("\n");
}


/* directory helper functions */
struct ext2_dir_entry_2 *traverse(char *path, struct ext2_inode *root,
								  unsigned char *disk){
	struct ext2_dir_entry_2 *entry;
	struct ext2_inode* cur_inode;
	char type;
	const char del = '/';
	char *cur_path = strtok(path, &del);
	cur_inode = root;
	
	// check for correctness of root name
	if(strcmp(strtok(NULL, &del), "~") != 0){
		fprintf(stderr, "No such file or directory\n");
		return NULL;
	}else{
		while (cur_path != NULL){
			cur_path = strtok(NULL, &del);
			// for each filename in the path
			cur_inode = find_file_inode(cur_inode, cur_path, disk);
			//no filename matches in the whole directory, print and return error
			if(cur_inode == NULL){
				fprintf(stderr, "No such file or directory\n");
				return NULL;
				//return ENOENT;
			}
			// check the founded inode is a directory or not
			// if its directory, then continue to last
			// if its a file, then check if its the last one in the path
				// if yes, then just print the name and return
				// if not, then wrong file path, print error and return
			
			/* if current i_node is a directory
			if(check_inode_type(cur_inode) == 'd'){
				// get the corresponding dir_entry
				entry = cur 
				// check if the name matches the givin path
			
			}*/
			
		}
	}
	return NULL;
}

/* return the inode NUMBER of the file */
struct ext2_inode *find_file_inode(struct ext2_inode *dir_node,
							char *filename, unsigned char *disk){
	int cur_size, dir_size, block_count;
	struct ext2_dir_entry_2 *entry;
	dir_size = dir_node->i_size;
	entry = (struct ext2_dir_entry_2 *) 
			(disk + (dir_node->i_block[0]) * EXT2_BLOCK_SIZE);
	block_count = 1;
	// traverse through the block to check each entry
	while(cur_size < EXT2_BLOCK_SIZE){
		// current entry is the file we looking for
		if(strcmp(entry->name, filename) == 0){
			return get_inode(entry->inode, disk);
		}else{
			// look up to next entry, update variables
			entry = (void *) entry + entry->rec_len;
			cur_size = cur_size + entry->rec_len;
			// if the dirrectory size of more than 1 block
			if(cur_size > EXT2_BLOCK_SIZE && 
						(cur_size + block_count * EXT2_BLOCK_SIZE)< dir_size){
				// reset cur_size, entry points to next block, block_count++
				cur_size = 0;
				block_count++;
				entry = (struct ext2_dir_entry_2 *)
						(disk + 
						(dir_node->i_block[0] + block_count) * EXT2_BLOCK_SIZE);
			}
		}
	}

	return NULL;
}

char check_inode_type(struct ext2_inode *node){
	char type;
	if(S_ISLNK(node->i_mode)){
		type = 'l';
	}else if(S_ISDIR(node->i_mode)){
		type = 'd';
	}else{
		type = 'f';
	}
	return type;
}







































