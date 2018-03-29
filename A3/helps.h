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
#define USING 1
#define FREE 0
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

const char root_symbol = '/';

struct ext2_inode *find_file_inode(struct ext2_inode *,char *, unsigned char *);
char check_inode_type(struct ext2_inode *);
int real_entry_size(struct ext2_dir_entry_2 *);							
void print_block_content(int, int, unsigned char *);

/* get this and that and these and those and what and that and this and that */					
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

struct ext2_dir_entry_2 *get_entry(int block_number, unsigned char *disk){
	return (struct ext2_dir_entry_2 *) 
		   (disk + (block_number) * EXT2_BLOCK_SIZE);
}

char *get_block(int number, unsigned char *disk){
	return (char *) get_entry(number, disk);
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

int free_position(int *mp, int mode){
	int res, i;
	res = -1;
	for(i=0; i<mode; i++){
		if(mp[i] == 0){
			return i+1;
		}
	}
	return res;
}
/* bug proved----------------------------------------------------------------*/

/* ----------------  directory helper functions  ---------------------------- */

/* return get the first filename in the path
   @rest: the result of path after the first file
   @return: filename allocated on heap
 */
char *extract_filename(char *path, char **rest){
	char *result = malloc(sizeof(char) * strlen(path)+1);
	char *ptr;
	// cases: /thisd/thed/what.txt
	// 		  /
	//		  a.txt
	//		  /a.txt
	ptr = strchr(path, '/');
	// no '/' found in the given string, return the given string
	if(ptr == NULL){
		*(rest) = NULL;
		strcpy(result, path);
		result[strlen(path)]='\0';
		return result;
	}
	// there is at least one '/' in the given path
	else{
		// only root is given 
		if (strlen(path) == 1){
			*(rest) = NULL;
			//strcpy(result, path);
			//result[strlen(path)]='\0';
			result = NULL;
			return result;
		}
		// if the file is in the middle of the path, then there is another '/'
		char *temp;
		temp = strchr(ptr + 1, '/');
		// the file is the last name in the given path
		if(temp == NULL){
			*(rest) = NULL;
			strcpy(result, (ptr+1));
			result[strlen(ptr)-1]='\0';
			return result;
		}else{
			int file_len = strlen(path) - strlen(temp);
			//[file_len];
			strncpy(result, (ptr+1), file_len - 1);
			result[file_len] = '\0';
			(*rest) = temp;
			return result;
		}
	}
}

/* given a absolute path of a file, return its parent directory path
   and the filename, both allocated on the heap
   @path: absolute path of a file
   @dir: returned dir path of the file, allocated on the heap
   @filename: returned filename of the file, allocated on the heap
*/
int get_dir_filename(char *path, char **dir, char **filename){
	char *temp;
    temp = strrchr(path, '/');
    // cases: /dirname
    //        /dirname/
    //        /path1/dirname
    //        /path1/dirname/
    //        /  (invalid)
    if(path[0] != '/' || strlen(path) == 1){
      fprintf(stderr, "only gives root, no dirname in the argument\n");
      return -1;
    }
    else if(strlen(temp) == 1){// '/' followed by the filename
      char *pre = temp-1;
      while ((*pre) != '/'){
        pre--;
      }
      int name_len = strlen(pre) - strlen(temp) - 1;
      (*filename) = malloc(sizeof(char) * name_len + 1);
      strncpy((*filename), pre+1, name_len);
      (*filename)[name_len] = '\0';
      pre++; // pre now points to the first letter of the new directory name
      int dir_len = strlen(path) - strlen(pre);
      (*dir) = malloc(sizeof(char) * dir_len + 1);
      strncpy((*dir), path, dir_len);
      path[dir_len] = '\0';
      return 0;
    }else{
      int name_len = strlen(temp) - 1;
      (*filename) = malloc(sizeof(char) * name_len + 1);
      strncpy((*filename), (temp+1), name_len);
      (*filename)[name_len] ='\0';

      temp++;
      int dir_len = strlen(path) - strlen(temp);
      (*dir) = malloc(sizeof(char) * dir_len + 1);
      strncpy((*dir), path, dir_len);
      (*dir)[dir_len] = '\0';
      return 0;
    }
}
/* return the inode pointer that points to the inode represented by the last
 * file of the path, return NULL if the path is invalid
 */
struct ext2_inode *traverse(char *path, struct ext2_inode *root,
								  unsigned char *disk){
	printf("----TRAVERSE: get path: %s\n",path);
	struct ext2_inode* cur_inode;
	char type, *filename, *rest_path, full_path[strlen(path)+1];
	strcpy(full_path, path); full_path[strlen(path)] = '\0';

	filename = extract_filename(full_path, &rest_path);
	printf("extracted file %s in the path: %s\n", filename, full_path);
	cur_inode = root;
	
	printf("traversing the path: %s\n", path);
	// check for correctness of root name
	if(path[0] != root_symbol){
		printf("wrong root symbol\n");
		return NULL;
	}else{
		printf("the next filename in the path: %s\n", filename);
		// if no path follow the root, return root
		if(filename == NULL){
			printf("return the root node to print\n");
			free(filename);
			return root;
		}
		
		// keep checking until last filename in the path
		while (filename != NULL){
			printf("finding the inode for the next file in the path...\n");
			// for each filename in the path
			cur_inode = find_file_inode(cur_inode, filename, disk);
			printf("updated and checking the result of the inode returned\n");
			//no filename matches in the whole directory, print and return error
			if(cur_inode == NULL){
				printf("Cant find the inode under this dir, return NULL\n");
				free(filename);
				return NULL;
			}else{
				printf("found the inode corresponding to file '%s'\n", filename);
				//cur_path = strtok(NULL, &del);
				
				// printf("next filename %s\n", filename);
				type = check_inode_type(cur_inode);
				//printf("The next file in the path is: %s\n", filename);
				// cur_inode is the i_node representd by the filename
				// check the founded inode is a directory or not
				// if its directory, then continue to last
				if(type == 'f'){
					if(rest_path != NULL){
						printf("A file in the middle of the path, return NULL\n");
						// a file in the middle of the path, which is invalid
						free(filename);
						return NULL;
					}else{
						printf("The last file is a file, return the file inode.\n");
						// reached the last file, and it's a file
						// return the i_node of this file
						free(filename);
						return cur_inode;
					}
				}
				// if its a directory
				else{
					
					// if its the last directory, return
					if(rest_path == NULL || (strcmp(rest_path, "/") == 0)){
						printf("the last file is directory, return the dir inode\n");
						free(filename);
						return cur_inode;
					}
					// if not then keep going
					printf("rest_path: '%s'\n", rest_path);
					int i;
					for(i=0; i<strlen(rest_path); i++){
						full_path[i] = rest_path[i];
					}
					//strcpy(full_path, rest_path);
					full_path[strlen(rest_path)]='\0';
					printf("full path: '%s'\n ",full_path);
					filename = extract_filename(full_path, &rest_path);
					printf("get the dir node, check for next file in the dir %s\n", filename);
				}
			}
		}
		printf("cant find the whole path in the root, return null\n");
		free(filename);
		return NULL;
	}
}

/*  return the inode of the file 
	@dir_node: the inode of the parent directory
	@filename: the name of the file we want to find under the parent directory
	@return: the inode of the file, or NULL if it doesn't exist in the directory
	CAN NOT find file in the indirect blocks
*/
struct ext2_inode *find_file_inode(struct ext2_inode *dir_node,
							char *filename, unsigned char *disk){
	int cur_size, dir_size, block_count, total_blk;
	char name[EXT2_NAME_LEN + 1];
	printf("	finding the inode for %s\n", filename); 
	struct ext2_dir_entry_2 *entry;
	dir_size = dir_node->i_size; total_blk = dir_size / EXT2_BLOCK_SIZE;
	printf("	size of this dir is: %d\n", dir_size);
	
	entry = get_entry(dir_node->i_block[0], disk);
		
	printf("	dir entry allocated at block %d\n", dir_node->i_block[0]);
	block_count = 1;
	cur_size = 0;
	
	// traverse through the block to check each entry
	while((cur_size + entry->rec_len) <= EXT2_BLOCK_SIZE){
		// check the name if this entry
		memcpy(name, entry->name, entry->name_len);
		name[entry->name_len] = '\0';
		printf("	the name of this entry is %s\n", name);
		printf("	the rec_len of this entry is %d\n", entry->rec_len);
		// current entry is the file we looking for
		if(strcmp(name, filename) == 0){
			printf("	matched the entry name with target: %s, return the node\n", filename);
			return get_inode(entry->inode, disk);
		}else{
			// check if this entry is the last file in the block
			if(cur_size + entry->rec_len == EXT2_BLOCK_SIZE){
				// check if we searched the last block
				printf("blk count: %d, total blk: %d\n", block_count, total_blk);
				if(block_count >= total_blk){
					printf("reaches last block, return NULL\n");
					return NULL;
				}else{// go to the next block
					int next_blk = dir_node->i_block[block_count];
					entry = get_entry(next_blk, disk);
					cur_size = 0;
					block_count++;
				}
			}else{// looking for next entry in this block
				cur_size = cur_size + entry->rec_len;
				entry = (void *) entry + entry->rec_len;
			printf("	this entry doesn't match, update to next(cur_size: %d)\n", cur_size);
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


/* manipulating block contents */
void cp_block_content(int src, int tar, int size, unsigned char *disk){
	printf("cping content of block %d to block %d\n", src, tar);
	printf("before cping:\n");
	printf("\tcontent of src:\n");
	print_block_content(src, 20, disk);
	printf("\tcontent of target:\n");
	print_block_content(tar, 20, disk);
	char *fir = get_block(src, disk);
	char *sec = get_block(tar, disk);
	memcpy(sec, fir, size);
	printf("AFTER cping:\n");
	printf("\tcontent of src:\n");
	print_block_content(src, 20, disk);
	printf("\tcontent of target:\n");
	print_block_content(tar, 20, disk);
}

/* give a first entry of an entry block and a target file name
   @filename: the target filename in the entry block
   @enter: pointer to the first entry of the entry block
   @return the entry struct of that file 
   		   return NULL if no entry matches the filename
 */
struct ext2_dir_entry_2 *find_entry(char *filename,
									struct ext2_dir_entry_2 *enter){
	printf("geting the file entry of file %s\n",filename);
	char entry_name[EXT2_NAME_LEN + 1];
	int cur_size = 0;
	struct ext2_dir_entry_2 *res;
	res = enter;
	while((cur_size+res->rec_len) <= EXT2_BLOCK_SIZE){
		memcpy(entry_name, res->name, res->name_len);
		entry_name[res->name_len] = '\0';
		printf("checking entry: %s\n", entry_name);
		if(strcmp(filename, entry_name) == 0){
			return res;
		}
		else{
			// look up to next entry, update variables
			cur_size = cur_size + res->rec_len;
			res = (void *) res + res->rec_len;
		}
	}
	return NULL;
}

/* update the entry block by the given new info of a entry
   @blk_number: block number of the entry block
   @filetype: filetype of the updating entry
   @new_inode: inode number that related to the new entry
   @new_name: name of the new entry(file/directory)
   block must have enough space
*/
void update_entry_block(int blk_number, unsigned char filetype,
						int new_inode, char *new_name, unsigned char *disk){

	printf("\n\tUPDATING dir entry block %d\n", blk_number);
	struct ext2_dir_entry_2 *entry, *update;

	entry = get_entry(blk_number, disk);
	int cur_size = 0;
	char entry_name[EXT2_NAME_LEN + 1];


	/* deal with unpredicable rec_len of '..' in case of dir is not empty */
	// skip '.' entry
	entry = (void *) entry + entry->rec_len;
	cur_size = cur_size + entry->rec_len;

	// if the directory is empty. (rec_len in '..' is huge)
	if((cur_size + entry->rec_len) == EXT2_BLOCK_SIZE){
		update = (void *) entry + real_entry_size(entry);
	

		update->inode = new_inode;			
		update->name_len = strlen(new_name);
		memcpy(update->name, new_name, strlen(new_name));
		(update->name)[strlen(new_name)] = '\0';
		update->file_type = filetype;

		update->rec_len = (entry->rec_len) -(real_entry_size(update));
		entry->rec_len = real_entry_size(entry);
		printf("rec_len of new entry: %d, pre rec_len: %d\n",update->rec_len, entry->rec_len);
		return;
	}
	/* directory is not empty, however, sometime the rec_len of '..' is not same
	   as the calculated real size of '..', so need to skip the '..' entry and
	   then check the rest of entreis
	*/
	else{
		while (cur_size < EXT2_BLOCK_SIZE){
			// this entry is the last
			if((cur_size + entry->rec_len) == EXT2_BLOCK_SIZE){
				cur_size = cur_size + entry->rec_len;

				memcpy(entry_name, entry->name, entry->name_len);
				entry_name[entry->name_len] = '\0';
				printf("the last file in this directory is: %s ", entry_name);
				printf("with rec_len: %d, real_entry_size: %d\n",
					entry->rec_len, real_entry_size(entry));

				update = (void *) entry + real_entry_size(entry);

				update->inode = new_inode;			
				update->name_len = strlen(new_name);
				memcpy(update->name, new_name, strlen(new_name));
				update->file_type = filetype;

				update->rec_len = (entry->rec_len) -
								  (real_entry_size(update));
				entry->rec_len = real_entry_size(entry);
				printf("rec_len of new entry: %d, pre rec_len: %d\n",update->rec_len, entry->rec_len);
				break;
			}
			printf("entry has reclen: %d,name: %s\n", entry->rec_len,entry->name);
			// if not, keep looking
			cur_size = cur_size + entry->rec_len;
			entry = (void *) entry + entry->rec_len;

		}
	}
}

int real_entry_size(struct ext2_dir_entry_2 *ent){
	int size = sizeof(unsigned int) + sizeof(unsigned short)
		 	 + sizeof(unsigned char) + sizeof(unsigned char)
		 	 + ent->name_len;
	if(size%4 == 0){
		return size;
	}else{
		return size + (4 - size % 4);
	}
}

void print_block_content(int blk_number, int size, unsigned char *disk){
	char *fir = get_block(blk_number, disk);
	int i;
	printf("printing content of block %d\n", blk_number);
	for(i=0;i<size;i++){
		printf("%c", fir[i]);
	}
	printf("\n");
}
































