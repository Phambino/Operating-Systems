#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#define CMAX 50

struct data_list {
    char *data;
    struct data_list *next_d;
};

struct inst_list {
    char *inst;
    struct inst_list *next_i;
};

void print_ll(struct data_list d, struct inst_list is) {
    printf("Instructions: ");
    while(is !=NULL) {
	printf("%s\n", is->inst);
	is = is->next_i;
    }
    printf("Data: \n");
    while(d != NULL) {
	printf("%s\n", d->data);
	d = d->next;
    }
}

int main(int argc, char** argv) {
    FILE *f;
    char str[CMAX];
    char *ins;
    char *data;
    char data2[CMAX];
    int dlen;
    struct data_list *dlist = malloc(sizeof(struct data_list));
    struct inst_list *ilist = malloc(sizeof(struct inst_list));

    dlist->data = malloc(sizeof(char*)*CMAX);
    ilist->inst = malloc(sizeof(char*)*CMAX);

    f = fopen(argv[1],"r");
    if(f == NULL) {
	printf("Error open file");
	return 1;
    }
    struct data_list *temp = malloc(sizeof(struct data_list));
    temp->data = malloc(sizeof(char*)*CMAX);

    while(fgets(str,CMAX,f) != NULL){
	data = strtok(str,",");
	ins = strtok(NULL,",");
	strncpy(data2,data,sizeof(data2));
	dlen = strlen(data2) - 3;
	data2[dlen] = '\0';
        strcpy(temp->data, data2);
	if(strcmp(ins,"I\n") == 0) {
	    if(strcmp(ilist->inst,"")==0) {
		strcpy(ilist->inst, temp->data);
	    } else{
		ilist->next_i = malloc(sizeof(struct inst_list));
		ilist->next_i->inst = malloc(sizeof(char*)*CMAX);
		temp->next_d = malloc(sizeof(struct data_list));
		temp->next_d->data = malloc(sizeof(char*)*CMAX);
		ilist->next_i->inst = temp->data;
		ilist = ilist->next_i;
		temp = temp->next_d;	
	    }
	} else {
	    if(strcmp(dlist->data,"") == 0) {
	 	strcpy(dlist->data, temp->data);
	    } else {
		dlist->next_d = malloc(sizeof(struct data_list));
		dlist->next_d->data = malloc(sizeof(char*)*CMAX);
		temp->next_d = malloc(sizeof(struct data_list));
		temp->next_d->data = malloc(sizeof(char*)*CMAX);
		dlist->next_d->data = temp->data;
		dlist = dlist->next_d;
		temp = temp->next_d;
	    }
	}
    }
    fclose(f);

    print_ll(ilist, dlist);
    return 0;
}
