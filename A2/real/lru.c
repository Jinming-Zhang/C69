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
int i;
/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
  int res = stack[0];
  int j;
  for(j=1; j<memsize; j++){
    stack[j-1] = stack[j];
  }
  i--;
  return res;
}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {
  // get the physical frame stored in p
  int frame_number = p->frame >> PAGE_SHIFT;
  int j;
  for(j = 0; j < i; j++){
    if (stack[j] == frame_number){
      // move j to the newest accessed in the
      int idx;
      for(idx=j; idx<i; idx++){
	stack[idx] = stack[idx+1];
      }
      stack[i-1] = frame_number;
      return;
    }
  }
  stack[i] = frame_number;
  i++;
  return;
}


/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void lru_init() {
  stack = malloc(sizeof(int) * memsize);
  i = 0;
}
