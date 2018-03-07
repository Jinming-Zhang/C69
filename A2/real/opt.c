#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

extern char *tracefile;

int curr_line;
int table_zise;
int MAXLINE = 256;

//my data type for implement hashtable
struct node
{
	addr_t vaddr;
	struct node *next;
	int distance;
};

struct slot
{
	struct node *first;
	struct node *last;
};
//my hashtable
struct slot **hashtable;
int *hashbits; //indicate hashtable slot empty or not

//below are my helper functions for implement hash table

//get the hash index of address
//i choose to used the division method
//since it is easy to implement 
int gethash(addr_t vaddr){
	return vaddr % table_zise;
}
//creat new node
struct node* init_node(addr_t vaddr, int distance){
	struct node *curr = (struct node *) malloc(sizeof(struct node));
	curr->distance = distance;
	curr->vaddr = vaddr;
	curr->next =NULL;
	return curr;
}

//create new slot
struct slot* init_slot(){
	struct slot *new = (struct slot *) malloc(sizeof(struct slot));
	new->first=NULL;
	new->last=NULL;
	return new;
}
//add node in the hashtable slots
//if no node in given slot, create a new node 
//else add it to the end, update relevant field
void add_node(addr_t vaddr, int index){
	//find vaddr hash location
	//i choose to use the division method to get hash value
	int loc = gethash(vaddr);
	struct node *new = init_node(vaddr,index);

	if(hashtable[loc]->first!=NULL){
		hashtable[loc]->last->next = new;
		hashtable[loc]->last = new;
	}else{
		hashtable[loc]->first = new;
		hashtable[loc]->last = new;
	}
	return;
}


/* Page to evict is chosen using the optimal (aka MIN) algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int opt_evict() {
	int i;
	int location;
	int index;
	int max_distance = -1;
	for(i=0;i<memsize;i++){
		index = gethash(coremap[i].vaddr);
		//no such vaddr in hashtable
		if(hashbits[index]!=0){
			printf("going to evict %d\n", i);
			return i;
		}
		//process the linked list in slot
		if(hashtable[index]->first!=NULL){
			if(hashtable[index]->first->distance > max_distance){
				max_distance = hashtable[index]->first->distance;
				location = i;
			}
		}else{
			return i;
		}
	}
	return location;
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {
	int frame = p->frame >> PAGE_SHIFT;
	int index  = gethash(coremap[frame].vaddr);
	if(hashtable[index]->first!=NULL){
		struct node *remove = hashtable[index]->first->next;
		hashtable[index]->first = remove;
		free(remove);
	}
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {
	FILE *fp;
	char buf[MAXLINE];
	char type;
	int table_size = 0;
	addr_t vaddr= 0;
	//open file
	fp = fopen(tracefile,"r");
	if(fp==NULL){
		perror("Open tracefile error:");
		exit(1);
	}

	//get file line number
	while(fgets(buf, MAXLINE,fp)!=NULL){
		//remove comments line
		if(buf[0]!='='){
			table_size++;
		}
	}

	//init hashtable
	hashtable = malloc(sizeof(struct slot)*table_size);
	hashbits = malloc(sizeof(int)*table_size);

	//reset pf to the head of file
	fseek(fp, 0, SEEK_SET);

	//init indication array
	int i=0;
	for(i=0;i<table_size; i++){
		hashbits[i]=0;
		hashtable[i] = init_slot();
	}

	// second time read file to update hash table
	while(fgets(buf,MAXLINE,fp)!=NULL){
		if (buf[0]!='='){
			sscanf(buf, "%c %lx", &type,&vaddr);
			add_node(vaddr,i);
			hashbits[gethash(vaddr)]=1;
			i++;
		}
	}

	if(fclose(fp)!=0){
		perror("tracefile close error:");
	}
}

