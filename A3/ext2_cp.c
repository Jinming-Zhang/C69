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
    struct ext2_inode *root, *src_file, *tar_dir;
	char *file_path, *cp_path, *new_file_name;
	int i, overwrite;
	overwrite = FALSE;
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
	    perror("mmap");
	    exit(1);
    }
    
    /* validate and parse commands */
    // disk.img path
    if(argc == ARGUMENTS){
        file_path = argv[2];
        cp_path = argv[3];
    }
    else{
        printf("Invalid arguments\n");
        return 0;
    } 
   
    /*printing each directory on a separate line.*/
    root = get_inode(EXT2_ROOT_INO, disk);
	// get the first path in after the root node
	src_file = traverse(file_path, root, disk);
	tar_dir = traverse(cp_path, root, disk);

	if(src_file == NULL){
		fprintf(stderr, "No such file or directory\n");
		return ENOENT;
	}
	// get the target directory inode and the filename regards to command
	if(tar_dir == NULL){// maybe a path to a renamed file
		char *tar_dir_path;
		if(get_dir_filename(cp_path, &tar_dir_path, &new_file_name) != 0){
			fprintf(stderr, "Error on extracting dir name and file name.\n");
			return 0;
		}
		printf("cping file to dir %s with name %s\n", tar_dir_path, new_file_name);
		// try to get the target directory inode
		tar_dir = traverse(tar_dir_path, root, disk);
		if(tar_dir == NULL){
			fprintf(stderr, "No such file or directory\n");
			return ENOENT;
		}
	}else{// may cp to a directory or overwrite an existed file
		if(check_inode_type(tar_dir) != 'd'){// overwrite an existed file
			char *tar_dir_path;
			if(get_dir_filename(cp_path, &tar_dir_path, &new_file_name) != 0){
				fprintf(stderr, "Error on extracting dir name and file name.\n");
				return 0;
			}
			tar_dir = traverse(tar_dir_path, root, disk);
			overwrite = TRUE;
			printf("overwrite file to dir %s with name %s\n", tar_dir_path, new_file_name);
		}else{// the filename is same as src file
			char *src_dir_path;
			if(get_dir_filename(file_path, &src_dir_path, &new_file_name) != 0){
				fprintf(stderr, "Error on extracting dir name and file name.\n");
				return 0;
			}
			printf("cping file to dir %s with name %s\n", cp_path, new_file_name);
			free(src_dir_path);
		}
	}

	// allocate necessary resources
	int blk_bitmap[BLOCKS], ind_bitmap[INODES], free_blk, free_ind;
	get_bitmap(blk_bitmap, disk, BLOCKS);
	get_bitmap(ind_bitmap, disk, INODES);
	if(overwrite == FALSE){
		free_ind = free_position(ind_bitmap, INODES);
		set_bitmap(ind_bitmap, disk, INODES, free_ind, USING);
		printf("find free inode at postion %d\n", free_ind);
	}else{
		int last_blk;
		last_blk = last_dir_blk(tar_dir, disk);
		printf("computed last dir entry block: %d, actual block: %d\n",
			last_blk, tar_dir->i_block[0]);
		struct ext2_dir_entry_2 *tar_dir_ent;
		tar_dir_ent = get_entry(last_blk, disk);
		tar_dir_ent = find_entry(new_file_name, tar_dir_ent);
		free_ind = tar_dir_ent->inode;
		printf("get the tar inode: %d\n", free_ind);
	}

	// set up new inodes for the cp file
	struct ext2_inode *dest_inode = get_inode(free_ind, disk);
	dest_inode->i_mode = EXT2_S_IFREG;
	dest_inode->i_size = src_file->i_size;
	dest_inode->i_links_count = 1;
	dest_inode->i_blocks = src_file->i_blocks;

	// set up blocks to cping the src file contents
	int file_blk, total_blk;
	// get the total blocks used to store the file content
    total_blk = total_blks(src_file->i_blocks, disk);

	for(i=0; i<total_blk; i++){
		file_blk = src_file->i_block[i];
		free_blk = free_position(blk_bitmap, BLOCKS);
		printf("find free block at %d\n", free_blk);
		dest_inode->i_block[i] = free_blk;
		cp_block_content(file_blk, free_blk, EXT2_BLOCK_SIZE, disk);
		set_bitmap(blk_bitmap, disk, BLOCKS, free_blk, USING);
	}

	// only update the target directory's entry when it's not overwriting
	if(overwrite == FALSE){
		int last_blk;
		last_blk = last_dir_blk(tar_dir, disk);


		printf("tar dir entry block is %d\n", last_blk);
		update_entry_block(last_blk, 
						   EXT2_FT_REG_FILE, free_ind, new_file_name, disk);
	}


	// copying contents of original file block to new one
	
	printf("\nFinish cping file...\n");
	printf("new blk bitmap:\n");
	print_bitmap(blk_bitmap, BLOCKS);
	printf("new ind bitmap:\n");
	print_bitmap(ind_bitmap, INODES);
	


	
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
    return 0;
}
