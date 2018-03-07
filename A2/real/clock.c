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
/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {
  int res;
	int i = 0;
	pgtbl_entry_t *pte;
	while (i<idx){
		// get the refbit of the pte points to first frame number
		pte = coremap[stack[i]].pte;
		int reffed = pte->frame & PG_REF;
		//printf("ref of frame at %d: %d\n", i, reffed);
		// frame is refered
		if(reffed){
			// reset ref and keep going
			pte->frame = pte->frame & (~PG_REF);
			//printf("reseting the ref of frame at %d: %d\n", i, pte->frame & PG_REF);
			if(i==idx-1){
				i=0;}else{i++;}
		}
		// frame is not refered -> evict the frame
		else{
			res = stack[i];
			//swap
			int j;
			for(j=i; j<memsize-1; j++){
				stack[j] = stack[j+1];
			}
			break;
		}	
	}
	idx--;
	//printf("current idx after evict%d\n", idx);
	return res;
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {
	int i;
	for(i=0; i<idx; i++){
		if(stack[i] == (p->frame >> PAGE_SHIFT)){
			pgtbl_entry_t *pte;
			pte = coremap[stack[i]].pte;
			pte->frame = pte->frame | PG_REF;
			//printf("now set the ref to 1 of frame %d, %d\n", stack[i], pte->frame & PG_REF);
			return;
		}
	}
	stack[idx] = p->frame >> PAGE_SHIFT;
	idx++;
		//printf("current idx after ref%d\n", idx);
}

/* Initialize any data structures needed for this replacement
 * algorithm. 
 */
void clock_init() {
	stack = malloc(sizeof(int) * memsize);
	idx = 0;
}
