#include "queue.h"

// To create a queue
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

// To Enqueue a car
int queue_put(queue *q, struct car *nuevo_coche) {
  pthread_mutex_lock(&q->mutex);

  while (q->count == q->size) {
    pthread_cond_wait(&q->cond_not_full, &q->mutex);
  }

  q->data[q->tail] = *nuevo_coche;
  q->tail = (q->tail + 1) % q->size;
  q->count++;

  pthread_cond_signal(&q->cond_not_empty);
  pthread_mutex_unlock(&q->mutex);

  return 0;
}

// To Dequeue a car
struct car *queue_get(queue *q) {
  pthread_mutex_lock(&q->mutex);

  while (q->count == 0) {
    pthread_cond_wait(&q->cond_not_empty, &q->mutex);
  }

  struct car *coche_salida = (struct car *)malloc(sizeof(struct car));
  *coche_salida = q->data[q->head];
  q->head = (q->head + 1) % q->size;
  q->count--;

  pthread_cond_signal(&q->cond_not_full);
  pthread_mutex_unlock(&q->mutex);

  return coche_salida;
}

// To check queue state
int queue_empty(queue *q) {
  pthread_mutex_lock(&q->mutex);
  int resultado = (q->count == 0);
  pthread_mutex_unlock(&q->mutex);
  return resultado;
}

int queue_full(queue *q) {
  pthread_mutex_lock(&q->mutex);
  int resp = (q->count == q->size);
  pthread_mutex_unlock(&q->mutex);
  return resp;
}

// To destroy the queue and free the resources
int queue_destroy(queue *q) {
  pthread_mutex_destroy(&q->mutex);
  pthread_cond_destroy(&q->cond_not_full);
  pthread_cond_destroy(&q->cond_not_empty);
  free(q->data);
  free(q);
  return 0;
}
