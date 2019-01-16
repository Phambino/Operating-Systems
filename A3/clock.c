#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;
int clock_index; 
/* Page to evict is chosen using the clock algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int clock_evict() {
	while(1){ // clock continously moves
	if ((coremap[clock_index].pte->frame & PG_REF) != PG_REF){ 
				return clock_index;
			}
			coremap[clock_index].pte->frame =  coremap[clock_index].pte->frame & ~PG_REF;
			clock_index = (int)(clock_index + 1) % memsize;
	}
	return 0;
}

/* This function is called on each access to a page to update any information
 * needed by the clock algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void clock_ref(pgtbl_entry_t *p) {
   	p->frame = p->frame | PG_REF; //set ref for second chance lookup
	return;
}

/* Initialize any data structures needed for this replacement
 * algorithm. 
 */
void clock_init() {
	clock_index = 0;
}