#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"



extern int memsize;
extern int debug;
extern char * tracefile;
extern struct frame *coremap;


struct trace {
	int index;
	addr_t vaddr;
    struct trace *next_trace;
};

int MAXSIZE;
struct trace **traces; // global list of nodes

//returns the file line 
int find (struct trace *tr){
	if(tr == NULL){
		return -1;
	}
	return tr->index;
}
// adds to the bottom of the node list at given index of traces 
int add(struct trace *head,addr_t v,int fl){
	struct trace *new = malloc(sizeof(struct trace));
	new->index = fl;
	new->next_trace = NULL;
	new->vaddr = v;

	struct trace *temp = head;
	if (temp == NULL){
		temp = new;
		return 0;
	}
	while(temp->next_trace != NULL){
		temp = temp->next_trace;
	}
	temp->next_trace = new;
	return 0;
	
	
}
//updates the the list of nodes to adds to the list of nodes at given index
int update(addr_t v){
	int index = PGDIR_INDEX(v)*PTRS_PER_PGTBL+PGTBL_INDEX(v); //formula for vadd to index for traces

	if(traces[index] == NULL){
		return 0; 
	}
	struct trace *temp = traces[index];
	if (temp->next_trace == NULL){
		traces[index] = NULL;
		free(temp);
		return 0;
	}
	traces[index] = temp->next_trace;
	free(temp);
	return 0;
}

/* Page to evict is chosen using the optimal (aka MIN) algorithm. 
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */
int opt_evict() {
	addr_t r = coremap[0].vaddr;
	int index = PGDIR_INDEX(r)*PTRS_PER_PGTBL + PGTBL_INDEX(r); //formula for vadd to index for traces
	int max = find(traces[index]);
	int frame = 0;
	if(max == -1){
				
		return 0;
	}
	for(int i = 1; i<memsize; i++){
		addr_t r = coremap[i].vaddr;
		index = PGDIR_INDEX(r)*PTRS_PER_PGTBL + PGTBL_INDEX(r);
		int check = find(traces[index]);
		if(check == -1){
			return i;
		}
		else if(max < check){
             max = check;
			 frame = i;
		}
	}
	return frame;
}

/* This function is called on each access to a page to update any information
 * needed by the opt algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void opt_ref(pgtbl_entry_t *p) {
    update(coremap[p->frame >> PAGE_SHIFT].vaddr);
	return;
}

/* Initializes any data structures needed for this
 * replacement algorithm.
 */
void opt_init() {
	MAXSIZE = PTRS_PER_PGDIR*PTRS_PER_PGTBL;
	traces = malloc(sizeof(struct trace *)*MAXSIZE);
	for(int i = 0;i<MAXSIZE;i++){
		traces[i] = NULL;
	}
	char buf[256];
    FILE *fp = fopen(tracefile,"r");
	int file_line = 0;
	char type;
	addr_t vadd;
	while (fgets(buf, 256, fp) != NULL){  // reading from tracfile similar to sim
		if(buf[0] != '='){
			sscanf(buf, "%c %lx", &type, &vadd);
			unsigned ind = PGDIR_INDEX(vadd)*PTRS_PER_PGTBL + PGTBL_INDEX(vadd); //formula for vadd to index for traces
        	if(traces[ind]  == NULL){ 
				struct trace *new_trace = malloc(sizeof(struct trace));
				new_trace->index = file_line;
				new_trace->next_trace = NULL;
				new_trace->vaddr = vadd;
				traces[ind] = new_trace;
			}
			else{
				add(traces[ind],vadd,file_line);
			}
		
		}
		file_line++;	
	}
    fclose(fp);



}