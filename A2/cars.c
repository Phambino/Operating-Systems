#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "traffic.h"
extern struct intersection isection;


/**
 * Populate the car lists by parsing a file where each line has
 * the following structure:
 *
 * <id> <in_direction> <out_direction>
 *
 * Each car is added to the list that corresponds with 
 * its in_direction
 * 
 * Note: this also updates 'inc' on each of the lanes
 */
void parse_schedule(char *file_name) {
    int id;
    struct car *cur_car;
    struct lane *cur_lane;
    enum direction in_dir, out_dir;
    FILE *f = fopen(file_name, "r");

    /* parse file */
    while (fscanf(f, "%d %d %d", &id, (int*)&in_dir, (int*)&out_dir) == 3) {

        /* construct car */
        cur_car = malloc(sizeof(struct car));
        cur_car->id = id;
        cur_car->in_dir = in_dir;
        cur_car->out_dir = out_dir;

        /* append new car to head of corresponding list */
        cur_lane = &isection.lanes[in_dir];
        cur_car->next = cur_lane->in_cars;
        cur_lane->in_cars = cur_car;
        cur_lane->inc++;
    }

    fclose(f);
}

/**
 * TODO: Fill in this function
 *
 * Do all of the work required to prepare the intersection
 * before any cars start coming
 * 
 */
void init_intersection() {
  
  for (int i = 0; i< 4; i++){
    pthread_mutex_init(&isection.quad[i],NULL);
    pthread_mutex_init(&isection.lanes[i].lock,NULL);
    pthread_cond_init(&isection.lanes[i].producer_cv,NULL);
    pthread_cond_init(&isection.lanes[i].consumer_cv,NULL);
    isection.lanes[i].inc = 0;
    isection.lanes[i].passed = 0;
    isection.lanes[i].buffer = malloc(sizeof(struct car *)*LANE_LENGTH);
     isection.lanes[i].head = 0;
     isection.lanes[i].tail = 0;
    isection.lanes[i].capacity = LANE_LENGTH;
    isection.lanes[i].in_buf = 0;
    isection.lanes[i].in_cars = NULL;
    isection.lanes[i].out_cars = NULL; 
   }
}

/**
 * TODO: Fill in this function
 *
 * Populates the corresponding lane with cars as room becomes
 * available. Ensure to notify the cross thread as new cars are
 * added to the lane.
 * 
 */
void *car_arrive(void *arg) {
    
    struct lane *l = arg;

    /* avoid compiler warning */
    l = l;
    while(l->in_cars != NULL){
    pthread_mutex_lock(&l->lock);
    // check if list is full if it is wait while till car cross takes one out of buffer 
    while(l->in_buf == l->capacity){
         pthread_cond_wait(&l->consumer_cv,&l->lock); 
    }
    // grab car from in_cars 
    struct car * temp = l->in_cars;
    l->in_cars = l->in_cars->next;
    l->buffer[l->tail] = temp;
    l->in_buf++;    
    l->tail++;
   // this allows for wrap around 
    if (l->tail == l->capacity){
       l->tail=0;
    }
    //signal car_cross that you added a car that needs to cross
     pthread_cond_signal(&l->producer_cv);
    pthread_mutex_unlock(&l->lock);
    }
     return NULL; 
}

/**
 * TODO: Fill in this function
 *
 * Moves cars from a single lane across the intersection. Cars
 * crossing the intersection must abide the rules of the road
 * and cross along the correct path. Ensure to notify the
 * arrival thread as room becomes available in the lane.
 *
 * Note: After crossing the intersection the car should be added
 * to the out_cars list of the lane that corresponds to the car's
 * out_dir. Do not free the cars!
 *
 * 
 * Note: For testing purposes, each car which gets to cross the 
 * intersection should print the following three numbers on a 
 * new line, separated by spaces:
 *  - the car's 'in' direction, 'out' direction, and id.
 * 
 * You may add other print statements, but in the end, please 
 * make sure to clear any prints other than the one specified above, 
 * before submitting your final code. 
 */
void *car_cross(void *arg) {
   struct lane *l = arg;

    // avoid compiler warning 
    l = l;
    while(l->inc >0){
    pthread_mutex_lock(&l->lock);

    //wait until car_arrive adds a car to the buffer 
    while(l->in_buf == 0){
     pthread_cond_wait(&l->producer_cv,&l->lock);
    }
    
   
   //grab from buffer leave lane and signal car arrive that you took a car out
    struct car *move_car;
    int head = l->head;
    
    move_car = l->buffer[head];
    l->head++;
    l->inc--;
    l->in_buf--;
    //for wraping head
    if(l->head == l->capacity){
      l->head = 0;
    }
     
    pthread_cond_signal(&l->consumer_cv);
    pthread_mutex_unlock(&l->lock);
     //call on cumpute_path and lock the corresponding quadrands
    int *com_list = compute_path(move_car->in_dir,move_car->out_dir); 
    for(int i = 0 ; i< 4; i++){
        int f = com_list[i];
        if( f > -1){
        pthread_mutex_lock(&isection.quad[f]);
        }  
    }  
   
    // add the car to the out_car list in the cars out_dir lane
    pthread_mutex_lock(&isection.lanes[move_car->out_dir].lock);
    move_car->next = isection.lanes[move_car->out_dir].out_cars;
    isection.lanes[move_car->out_dir].out_cars = move_car;
    isection.lanes[move_car->out_dir].passed++;
    pthread_mutex_unlock(&isection.lanes[move_car->out_dir].lock);
    printf("id %d from %d to %d \n",move_car->id,move_car->in_dir,move_car->out_dir);
    
    //sucessfully moved the car so now you should unlock the quadrants
    for(int g = 0 ; g< 4; g++){
        int h = com_list[g];
        if(h > -1){
        pthread_mutex_unlock(&isection.quad[h]);
        }
    } 
    
    free(com_list);     
 }  
   return NULL;
 
}

/**
 * TODO: Fill in this function
 *
 * Given a car's in_dir and out_dir return a sorted 
 * list of the quadrants the car will pass through.
 * 
 */
int *compute_path(enum direction in_dir, enum direction out_dir) {
    int *quad_list = malloc(sizeof(int)*4);
    quad_list[0] = -1;  quad_list[1] = -1; quad_list[2] = -1; quad_list[3] = -1;
    //if in_dir > out_dir grab the lanes not between both dirs including out_dir
    if(in_dir > out_dir){
        for(int i = 0;i<out_dir+1; i++){
            quad_list[i] = i;
        }
        if(in_dir != 3){ //if its three thats the last lane don need to loop 
        for(int i = in_dir+1; i<4;i++){
            quad_list[i] = i;
        }
       }
       return quad_list;

     }
     //if out_dir>in_dir grab the lanes between both dir incliding in dir 
   else if(out_dir>in_dir){
       int size = out_dir - in_dir;
       int c = in_dir;
       for(int i = 0; i<size; i++){
           quad_list[i] = c;
           c++;
       }
       return quad_list;
   }
   // else they are equal, add all quads to the list
   else{
       quad_list[0] = 0;  quad_list[1] = 1; quad_list[2] = 2; quad_list[3] = 3;
       return quad_list;
   }
   
   

   return NULL;

}

