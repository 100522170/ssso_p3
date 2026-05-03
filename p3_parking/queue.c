#include "queue.h"

// builds the memory for the parking,
queue *queue_init(int size) {
  // we reserve the control structure
  queue *q = (queue *)malloc(sizeof(queue));

  // We reserve the array where cars are stored with the size of the car struct
  q->data = (struct car *)malloc((size_t)size * sizeof(struct car));
  q->head = 0;
  q->tail = 0;
  q->count = 0;
  q->size = size; // max capacity

  // POSIX tools
  // Initialize mutex
  pthread_mutex_init(&q->mutex, NULL);
  // Condition variables
  // (wait for the queue not to be full)
  pthread_cond_init(&q->cond_not_full, NULL);
  // (wait for the queue not to be empty in case we want to exit)
  pthread_cond_init(&q->cond_not_empty, NULL);

  return q;
}

// the function introduces a car in queue
int queue_put(queue *q, struct car *nuevo_coche) {
  pthread_mutex_lock(&q->mutex);

  // Wait if the queue is at capacity
  while (q->count == q->size) {
    pthread_cond_wait(&q->cond_not_full, &q->mutex);
  }

  // Copy car data into the queue at the current tail position
  q->data[q->tail] = *nuevo_coche;
  // Advance tail pointer with modulo wrap-around for circular behavior
  q->tail = (q->tail + 1) % q->size;
  // Increment the number of elements in queue
  q->count++;
  // Wake up a consumer waiting for elements
  pthread_cond_signal(&q->cond_not_empty);
  pthread_mutex_unlock(&q->mutex);
  return 0;
}
// Removes a car from the queue and wakes posible producers waiting
struct car *queue_get(queue *q) {
  pthread_mutex_lock(&q->mutex);

  // Wait if the queue is empty
  while (q->count == 0) {
    pthread_cond_wait(&q->cond_not_empty, &q->mutex);
  }
  struct car *coche_salida = (struct car *)malloc(sizeof(struct car));
  // Copy car data from queue to the new memory
  *coche_salida = q->data[q->head];
  // Advance head pointer r
  q->head = (q->head + 1) % q->size;
  // Decrement the number of elements in queue
  q->count--;
  // Wake up a producer waiting for space
  pthread_cond_signal(&q->cond_not_full);
  pthread_mutex_unlock(&q->mutex);
  return coche_salida;
}
// To check queue state if its empty or full
int queue_empty(queue *q) {
  // Protectwith mutex
  pthread_mutex_lock(&q->mutex);
  int resultado = (q->count == 0);
  pthread_mutex_unlock(&q->mutex);
  return resultado;
}
int queue_full(queue *q) {
  // Protect with mutex
  pthread_mutex_lock(&q->mutex);
  int resp = (q->count == q->size);
  pthread_mutex_unlock(&q->mutex);
  return resp;
}
// To destroy the queue and free the resources
int queue_destroy(queue *q) {
  // Destroy synchronization primitives
  pthread_mutex_destroy(&q->mutex);
  pthread_cond_destroy(&q->cond_not_full);
  pthread_cond_destroy(&q->cond_not_empty);
  // Free resources
  free(q->data);

  free(q);
  return 0;
}
