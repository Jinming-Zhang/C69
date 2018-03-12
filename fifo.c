#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

int tank[memsize];
int idx;
/* Page to evict is chosen using the fifo algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int fifo_evict() {
  // go through physical frames and find the frame with lowest rank
  /*int lowest_ranked_frame;
  int lowest_rank = memsize + 1;
  int i;
  for(i = 0; i < memsize; i++){
    if(coremap[i].rank < lowest_rank){
      lowest_rank = coremap[i].rank;
      lowest_ranked_frame = i;
    }
  }
  return lowest_ranked_frame;*/
  int res = tank[0];
  int j;
  for(j=0; j<(memsize-1); j++){
  	tank[j] = tank[j+1];
  }
return res;
}

/* This function is called on each access to a page to update any information
 * needed by the fifo algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void fifo_ref(pgtbl_entry_t *p) {
  /*coremap[(p->frame) >> PAGE_SHIFT].rank = rank;
  rank++;
  if(rank > memsize){
    int i;
    for(i = 0; i < memsize; i++){
      coremap[(p->frame) >> PAGE_SHIFT].rank = coremap[(p->frame) >> PAGE_SHIFT].rank - memsize;
    }
    rank = 0;
  }
  //printf("updating the rank of frame %d to %d\n", (p->frame) >> PAGE_SHIFT, rank);
  return;*/
	tank[idx] = p->frame >> PAGE_SHIFT;
	idx++;
}

/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void fifo_init() {
  /*int i;
  for (i = 0; i < memsize; i++){
    coremap[i].rank = memsize + 1;
    //printf("initializing the rank of frame %d\n", i);
  }*/
  idx = 0;
}
