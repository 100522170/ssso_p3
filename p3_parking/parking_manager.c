#include "queue.h"
#include <ctype.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NUM_GATES 4
#define NUM_PRIORITIES 3
#define NUM_EXIT_GATES 2
#define NUM_ENTRY_THREADS (NUM_GATES * NUM_PRIORITIES)

// Parking lot structure to manage capacity and occupation
typedef struct {
  int capacity;
  int occupied;
  pthread_mutex_t mutex;
  pthread_cond_t cond[NUM_PRIORITIES];
  int waiting[NUM_PRIORITIES];
} Lot;

Lot parking_lot;

// Function to initialize the parking lot structure
void lot_init(Lot *l, int capacity) {
  l->capacity=capacity;
  l->occupied=0;

  for(int i=0;i<NUM_PRIORITIES;i++){
    l->waiting[i]=0;
    pthread_cond_init(&l->cond[i], NULL);
  }
  pthread_mutex_init(&l->mutex, NULL);
}

// Function to claim a space in the parking lot
void lot_claim_space(Lot *l, int priority, const char *plate) {
  pthread_mutex_lock(&l->mutex);

  l->waiting[priority]++;

  // Wait here if the parking is full or if there are cars with higher priority waiting
  while(l->occupied>=l->capacity || (priority>0 && l->waiting[0]>0) ||
         (priority>1 && l->waiting[1]>0)){
    pthread_cond_wait(&l->cond[priority], &l->mutex);
  }

  l->waiting[priority]--;
  l->occupied++;

  printf("[INFO] LOT CLAIMED %d/%d BY %s\n", l->occupied, l->capacity, plate);

  pthread_mutex_unlock(&l->mutex);
}

// Function to release a space in the parking lot
void lot_release_space(Lot *l, const char *plate) {
  pthread_mutex_lock(&l->mutex);

  l->occupied--;

  printf("[INFO] LOT RELEASED %d/%d BY %s\n", l->occupied, l->capacity, plate);

  // Wake up waiting threads strictly respecting their priority
  if(l->waiting[0]>0){
    pthread_cond_signal(&l->cond[0]);
  }else if(l->waiting[1]>0){
    pthread_cond_signal(&l->cond[1]);
  }else if(l->waiting[2]>0){
    pthread_cond_signal(&l->cond[2]);
  }

  pthread_mutex_unlock(&l->mutex);
}

// ========== MAIN AND THREADS ==========

// Global synchronization so threads wait for the main function to finish reading the file
pthread_mutex_t start_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t start_cond = PTHREAD_COND_INITIALIZER;
int start_flag = 0;

// Arrays for entry and exit queues
queue *entry_queues[NUM_GATES][NUM_PRIORITIES];
queue *exit_queues[NUM_EXIT_GATES];

// Arguments for input threads
typedef struct {
  int gate;
  int priority;
  queue *lane_queue;
} ThreadArgs;

// Arguments for output threads
typedef struct {
  int gate;
  queue *exit_queue;
} ExitThreadArgs;

// Function to check if a license plate is valid (e.g., 1111-AAA)
int is_valid_plate(const char *plate) {
  if(strlen(plate)!=8)
    return 0;
  for(int i=0;i<4;i++)
    if(!isdigit(plate[i]))
      return 0;
  if(plate[4]!='-')
    return 0;
  for(int i=5;i<8;i++)
    if(!isupper(plate[i]))
      return 0;
  return 1;
}

// Thread to manage car entries
void *entry_lane_thread(void *arg) {
  ThreadArgs *args = (ThreadArgs *)arg;
  if(!args){
    fprintf(stderr, "[ERROR] [entry_lane_thread] Arguments not valid\n");
    return (void *)-1;
  }

  int var_lane=args->priority;
  int var_gate=args->gate;

  printf("[LANE %d] [GATE %d] Waiting\n", var_lane, var_gate);

  // Block the thread until we receive the start signal from main
  pthread_mutex_lock(&start_mutex);
  while(start_flag==0){
    pthread_cond_wait(&start_cond, &start_mutex);
  }
  pthread_mutex_unlock(&start_mutex);

  while(1){
    // Extract a struct car from the queue
    struct car *c = (struct car *)queue_get(args->lane_queue);

    // Check for the "END" marker to stop the thread
    if(c==NULL || strcmp(c->plate, "END")==0){
      if(c!=NULL)
        free(c);
      break;
    }

    printf("[LANE %d] [GATE %d] Car %s\n", var_lane, var_gate, c->plate);

    // Try to get a parking space
    lot_claim_space(&parking_lot, var_lane, c->plate);

    int exit_gate=var_gate%NUM_EXIT_GATES;
    
    // Put the car in the exit queue
    queue_put(exit_queues[exit_gate], c);
    
    // Free local memory since we already placed the car in the next queue
    free(c);
  }

  printf("[LANE %d] [GATE %d] Ending\n", var_lane, var_gate);
  free(args);
  pthread_exit((void *)0);
}

// Thread to manage car exits
void *exit_gate_thread(void *arg) {
  ExitThreadArgs *args = (ExitThreadArgs *)arg;
  if(!args){
    fprintf(stderr, "[ERROR] [EXIT -1] Arguments not valid\n");
    return (void *)-1;
  }

  int exit_gate=args->gate;

  while(1){
    // Extract a struct car from the exit queue
    struct car *c = (struct car *)queue_get(args->exit_queue);

    // Check for the "END" marker to stop the thread
    if(c==NULL || strcmp(c->plate, "END")==0){
      if(c!=NULL)
        free(c);
      break;
    }

    printf("[EXIT %d] CAR %s\n", exit_gate, c->plate);

    // Free the parking space
    lot_release_space(&parking_lot, c->plate);
    
    // The car is leaving the parking lot, so we finally free its memory
    free(c);
  }

  printf("[EXIT %d] Ending\n", exit_gate);
  free(args);
  pthread_exit((void *)0);
}

int main(int argc, char *argv[]) {
  // Check command line arguments
  if(argc!=4){
    fprintf(stderr, "Usage: %s <input file> <queue size> <parking capacity>\n",
            argv[0]);
    return -1;
  }

  int queue_size=atoi(argv[2]);
  int parking_capacity=atoi(argv[3]);

  if(queue_size<=0){
    fprintf(stderr, "Error: queue size incorect\n");
    return -1;
  }
  if(parking_capacity<=0){
    fprintf(stderr, "Error: parking capacity incorrect\n");
    return -1;
  }

  // NOTE FOR PERSON C: Initialize the Lot structure here before threads
  lot_init(&parking_lot, parking_capacity);

  // Initialize circular queues for entries and exits
  for(int i=0;i<NUM_GATES;i++){
    for(int j=0;j<NUM_PRIORITIES;j++){
      entry_queues[i][j]=queue_init(queue_size);
    }
  }
  for(int i=0;i<NUM_EXIT_GATES;i++){
    exit_queues[i]=queue_init(queue_size);
  }

  // Create and launch entry threads
  pthread_t entry_tids[NUM_GATES][NUM_PRIORITIES];
  for(int i=0;i<NUM_GATES;i++){
    for(int j=0;j<NUM_PRIORITIES;j++){
      ThreadArgs *args = malloc(sizeof(ThreadArgs));
      args->gate=i;
      args->priority=j;
      args->lane_queue=entry_queues[i][j];

      if(pthread_create(&entry_tids[i][j], NULL, entry_lane_thread, args)!=0){
        perror("[ERROR] Creating entry thread");
        return -1;
      }
    }
  }

  // Create and launch exit threads
  pthread_t exit_tids[NUM_EXIT_GATES];
  for(int i=0;i<NUM_EXIT_GATES;i++){
    ExitThreadArgs *args = malloc(sizeof(ExitThreadArgs));
    args->gate=i;
    args->exit_queue=exit_queues[i];

    if(pthread_create(&exit_tids[i], NULL, exit_gate_thread, args)!=0){
      perror("[ERROR] Creating exit thread");
      return -1;
    }
  }

  // Open and parse the CSV file
  int fd=open(argv[1], O_RDONLY);
  if(fd==-1){
    perror("[ERROR] [parking_manager] Opening file");
    return -1;
  }

  // ARRAY OF STRUCTURES: We store cars in memory first
  struct car *car_vector = malloc(1024*sizeof(struct car)); 
  int car_count=0;

  char c;
  ssize_t bytes_read;
  char line[256];
  int pos=0;
  int is_first_line=1;

  while((bytes_read=read(fd, &c, 1))>0){
    if(c=='\n' || c=='\r'){
      if(pos>0){
        line[pos]='\0';
        pos=0;

        // Skip the CSV header line
        if(is_first_line){
          is_first_line=0;
          continue;
        }

        char plate_buf[16];
        int gate_buf, priority_buf;

        if(sscanf(line, " %15[^,], %d , %d", plate_buf, &gate_buf, &priority_buf)==3){
          int len=(int)strlen(plate_buf);
          
          // Clean empty spaces at the end of the plate
          while(len>0 && isspace(plate_buf[len-1])){
            plate_buf[len-1]='\0';
            len--;
          }

          // If the car data is valid, we save it
          if(is_valid_plate(plate_buf) && gate_buf>=0 &&
              gate_buf<NUM_GATES && priority_buf>=0 &&
              priority_buf<NUM_PRIORITIES){
            
            if(car_count>=1024){
              fprintf(stderr, "Error: too many cars\n");
              continue;
            }
            strcpy(car_vector[car_count].plate, plate_buf);
            car_vector[car_count].gate=gate_buf;
            car_vector[car_count].priority=priority_buf;
            car_count++;
          }
        }
      }
    }else{
      if(pos<255)
        line[pos++]=c;
    }
  }

  if(bytes_read<0){
    perror("[ERROR] [parking_manager] Reading file");
    close(fd);
    return -1;
  }

  if(close(fd)==-1){
    perror("[ERROR] [parking_manager] Closing file");
    return -1;
  }

  // Signal the entry threads so they can start reading their queues
  pthread_mutex_lock(&start_mutex);
  start_flag=1;
  pthread_cond_broadcast(&start_cond);
  pthread_mutex_unlock(&start_mutex);

  // Put the cars we saved into their corresponding entry queues
  for(int i=0;i<car_count;i++){
    queue_put(entry_queues[car_vector[i].gate][car_vector[i].priority], &car_vector[i]);
  }

  // Send the "END" marker to entry queues so threads know when to stop
  for(int i=0;i<NUM_GATES;i++){
    for(int j=0;j<NUM_PRIORITIES;j++){
      struct car *end_car = malloc(sizeof(struct car));
      strcpy(end_car->plate, "END");
      queue_put(entry_queues[i][j], end_car);
    }
  }

  // Wait for all entry threads to finish processing cars
  for(int i=0;i<NUM_GATES;i++){
    for(int j=0;j<NUM_PRIORITIES;j++){
      void *ret;
      pthread_join(entry_tids[i][j], &ret);
      if(ret==(void *)-1){
        fprintf(stderr, "[ERROR][parking_manager] LANE %d of GATE %d FINISHED WITH ERROR\n", j, i);
      }
    }
  }

  // Entry is complete, now send the "END" marker to the exit queues
  for(int i=0;i<NUM_EXIT_GATES;i++){
    struct car *end_car = malloc(sizeof(struct car));
    strcpy(end_car->plate, "END");
    queue_put(exit_queues[i], end_car);
  }

  // Wait for all exit threads to finish
  for(int i=0;i<NUM_EXIT_GATES;i++){
    void *ret;
    pthread_join(exit_tids[i], &ret);
    if(ret==(void *)-1){
      fprintf(stderr, "[ERROR] [parking_manager] EXIT GATE %d FINISHED WITH ERROR\n", i);
    }
  }

  // Clean up and free the memory used by the queues
  for(int i=0;i<NUM_GATES;i++){
    for(int j=0;j<NUM_PRIORITIES;j++){
      queue_destroy(entry_queues[i][j]);
    }
  }
  for(int i=0;i<NUM_EXIT_GATES;i++){
    queue_destroy(exit_queues[i]);
  }

  // Free resources
  free(car_vector);
  pthread_mutex_destroy(&start_mutex);
  pthread_cond_destroy(&start_cond);

  printf("[OK][parking_manager] Finishing\n");
  return 0;
}