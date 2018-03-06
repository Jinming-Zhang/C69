#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

int *stack;
int idx;
/* Page to evict is chosen using the fifo algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int fifo_evict() {
	int res = stack[0];
	int i;
	for(i = 0; i<memsize-1; i++){
		stack[i] = stack[i+1];
	}
	idx--;
	return res;
}

/* This function is called on each access to a page to update any information
 * needed by the fifo algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void fifo_ref(pgtbl_entry_t *p) {
	int i;
	for(i=0; i<idx; i++){
		if(stack[i] == (p->frame >> PAGE_SHIFT)){
			return;
		}
	}
	stack[idx] = p->frame >> PAGE_SHIFT;
	idx++;
}

/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void fifo_init() {
	stack = malloc(sizeof(int) * memsize);
	idx = 0;
}
