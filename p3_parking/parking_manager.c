#include "queue.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUM_GATES 4
#define NUM_PRIORITIES 3
#define NUM_EXIT_GATES 2
#define NUM_ENTRY_THREADS (NUM_GATES * NUM_PRIORITIES)

typedef struct {
  int capacity;
  int occupied;
  pthread_mutex_t mutex;
  pthread_cond_t cond[NUM_PRIORITIES];
  int waiting[NUM_PRIORITIES];
} Lot;

Lot parking_lot;

// Arguments for input threads
typedef struct {
  int gate;
  int priority;
  queue *lane_queue;
} ThreadArgs;

int main(int argc, char *argv[]) {

  printf("[OK][parking_manager] Finishing\n");
  return 0;
}
