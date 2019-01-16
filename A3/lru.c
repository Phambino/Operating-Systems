#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "pagetable.h"


extern int memsize;

extern int debug;

extern struct frame *coremap;

//linked list struct for LRU system
struct node {
	int frame; 
	struct node* next; // pointer to next node
};
struct node *head;
struct node *tail;

//checks if node in the link list, if it is, remove it
int del_node(int num){
	struct node *temp = head;
	struct node *prev = NULL;
	while(temp != NULL){
		if(temp->frame == num){ //check if current node is the right frame and remove if it is
			if(prev == NULL){ //desired frame is at head
               head = head->next; //update
			   free(temp);
			    
			}
			else{ //between nodes
				prev->next = temp->next; //update
				free(temp);

			}
			return 0;
		}
		//on to the next node
		prev = temp;
		temp = temp->next;
	}
	return -1;
}

/* Page to evict is chosen using the accurate LRU algorithm.
 * Returns the page frame number (which is also the index in the coremap)
 * for the page that is to be evicted.
 */

int lru_evict() {
	struct node *temp = head;
	head  = head->next;
	temp->next = NULL;
	int frame = temp->frame;
	free(temp);  
	return frame;
}

/* This function is called on each access to a page to update any information
 * needed by the lru algorithm.
 * Input: The page table entry for the page that is being accessed.
 */
void lru_ref(pgtbl_entry_t *p) {
    int index = p->frame >> PAGE_SHIFT; 
	//if list is empty, create the head
	if(head == NULL && tail == NULL){
    	struct node *new = malloc(sizeof (struct node));
		new->frame = index;
		new->next = NULL;
		tail = new;
		head = tail;
	}
	//if list not empty, remove necessary node
    else{
		struct node* new_node = malloc(sizeof(struct node));
    	new_node->frame = index;
    	new_node->next = NULL;
		del_node(index);
		tail->next = new_node;
		tail = tail->next;      	
	}
	return;
}


/* Initialize any data structures needed for this 
 * replacement algorithm 
 */
void lru_init() {
	head = NULL;
	tail = NULL;

}
