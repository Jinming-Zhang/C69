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
	int i;

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
        printf("invalid commands\n");
        return 0;
    } 
   
    /*printing each directory on a separate line.*/
    root = get_inode(EXT2_ROOT_INO, disk);
	// get the first path in after the root node
	file = traverse(file_path, root, disk);
	destiny = traverse(cp_path, root, disk);
	if(file == NULL){
		fprintf(stderr, "No such file or directory\n");
		return ENOENT;
	}
	if(destiny == NULL){
		printf("renaming not implemented yet\n");
		return 0;
	}
	else{
		char tar_dir[strlen(cp_path)+1], tar_name[EXT2_NAME_LEN+1];
		char ori_dir[strlen(file_path)], ori_name[strlen(file_path)];

		// setup original file name and it's directory
		strcpy(ori_name, strrchr(file_path, '/') + 1);
		strncpy(ori_dir, file_path, strlen(file_path)-strlen(ori_name));
		ori_dir[strlen(file_path)-strlen(ori_name)]='\0';
		// check if the destination path is a directory or a renamed file
		if(check_inode_type(destiny) == 'd'){
			strcpy(tar_dir, cp_path);
			strcpy(tar_name, ori_name);
			printf("\nCPing to a directory: %s\n", tar_dir);
		}else{// cping to a directory with a new filename
			strcpy(tar_name, strrchr(tar_dir,'/')+1);
			strncpy(tar_dir, cp_path, strlen(cp_path)-strlen(tar_name));
		}
		
		printf("cping file '%s' under dir '%s' to '%s' under '%s'\n", ori_name, ori_dir, tar_name, tar_dir);
		
		struct ext2_inode *ori_dir_inode, *tar_dir_inode;
		printf("geting original dir inode\n");
		ori_dir_inode = traverse(ori_dir, root, disk);
		printf("geting target dir inode\n");
		tar_dir_inode = traverse(tar_dir, root, disk);
		/* test result tar_dir_node-----------------------*/
		struct ext2_dir_entry_2 *test = get_entry(tar_dir_inode->i_block[0],disk);
		printf("testing result tar_dir_inode.......\n");
		printf("entry name: %s, rec_len: %d, real size: %d\n", 
			test->name, test->rec_len, real_entry_size(test));
		test = (void *) test + test->rec_len;
		printf("entry name: %s, rec_len: %d, real size: %d\n", 
			test->name, test->rec_len, real_entry_size(test));
		test = (void *) test + test->rec_len;
		printf("entry name: %s, rec_len: %d, real size: %d\n", 
			test->name, test->rec_len, real_entry_size(test));
		test += test->rec_len;
		/* end test */

		// check available resources
		int blk_bitmap[BLOCKS], ind_bitmap[INODES];
		get_bitmap(blk_bitmap, disk, BLOCK_BITMAP);
		get_bitmap(ind_bitmap, disk, INODE_BITMAP);
		printf("blk bitmap:\n");
		print_bitmap(blk_bitmap, BLOCKS);
		printf("ind bitmap:\n");
		print_bitmap(ind_bitmap, INODES);
		/* to copy a file */
		// get the 
		int free_blk, free_ind;
		free_ind = free_position(ind_bitmap, INODE_BITMAP);
		printf("find free inode at postion %d\n", free_ind);

		/* update target directories entries */

		// update the target directory's entry
		struct ext2_dir_entry_2 *ori_entry;// *tar_entry;

		ori_entry = find_entry(ori_name, 
							  get_entry(ori_dir_inode->i_block[0], disk));
		//tar_entry = get_entry(tar_dir_inode->i_block[0], disk);
		printf("ori_entry information:\n");
		if(ori_entry == NULL){
			printf("cant find original file entry under the directory\n");
			return 0;
		}
		printf("entry inode: %d, entry filename: %s, entry rec_len: %d, entry real size :%d\n",
				ori_entry->inode, ori_entry->name,
				ori_entry->rec_len, real_entry_size(ori_entry));

		int tar_blk;
	    if((tar_dir_inode->i_blocks)%2 == 0){
		 	tar_blk = (tar_dir_inode->i_blocks)/2 - 1;
		}else{
			tar_blk = (tar_dir_inode->i_blocks)/2;
		}
		printf("tar dir entry block is %d\n", tar_dir_inode->i_block[0]);
		update_entry_block(tar_dir_inode->i_block[0],
						   ori_entry->file_type, free_ind, tar_name, disk);

		// setup new inode for destination
		struct ext2_inode *dest_inode = get_inode(free_ind, disk);
		dest_inode->i_mode = dest_inode->i_mode | EXT2_S_IFREG;
		dest_inode->i_size = get_inode(ori_entry->inode, disk)->i_size;
		//dest_inode->i_ctime = NULL;
		dest_inode->i_links_count = 1;
		dest_inode->i_blocks = get_inode(ori_entry->inode, disk)->i_blocks;

		set_bitmap(ind_bitmap, disk, INODE_BITMAP, free_ind, USING);

		// copying contents of original file block to new one
		int file_blk;
		for(i=0; i<=tar_blk; i++){
			file_blk = file->i_block[i];
			free_blk = free_position(blk_bitmap, BLOCK_BITMAP);
			printf("find free block at %d\n", free_blk);
			dest_inode->i_block[i] = free_blk;
			cp_block_content(file_blk, free_blk, EXT2_BLOCK_SIZE, disk);
			set_bitmap(blk_bitmap, disk, BLOCK_BITMAP, free_blk, USING);
		}
	printf("\nFinish cping file...\n");
	printf("new blk bitmap:\n");
	print_bitmap(blk_bitmap, BLOCKS);
	printf("new ind bitmap:\n");
	print_bitmap(ind_bitmap, INODES);
	}


	
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
    return 0;
}
