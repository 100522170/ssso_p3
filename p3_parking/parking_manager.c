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

// ========== SECCIÓN PERSONA B: ESTRUCTURA LOT ==========
typedef struct {
  int capacity;
  int occupied;
  pthread_mutex_t mutex;
  pthread_cond_t cond[NUM_PRIORITIES];
  int waiting[NUM_PRIORITIES];
} Lot;

Lot parking_lot;

// Función para inicializar la estructura Lot
void lot_init(Lot *l, int capacity) {
  l->capacity = capacity;
  l->occupied = 0;

  for (int i = 0; i < NUM_PRIORITIES; i++) {
    l->waiting[i] = 0;
    pthread_cond_init(&l->cond[i], NULL);
  }
  pthread_mutex_init(&l->mutex, NULL);
}

// Función para solicitar espacio en el parking
void lot_claim_space(Lot *l, int priority, const char *plate) {
  pthread_mutex_lock(&l->mutex);

  l->waiting[priority]++;

  // Esperar si está lleno o si hay coches de mayor prioridad esperando
  while (l->occupied >= l->capacity || (priority > 0 && l->waiting[0] > 0) ||
         (priority > 1 && l->waiting[1] > 0)) {
    pthread_cond_wait(&l->cond[priority], &l->mutex);
  }

  l->waiting[priority]--;
  l->occupied++;

  printf("[INFO] LOT CLAIMED %d/%d BY %s\n", l->occupied, l->capacity, plate);

  pthread_mutex_unlock(&l->mutex);
}

// Función para liberar espacio en el parking
void lot_release_space(Lot *l, const char *plate) {
  pthread_mutex_lock(&l->mutex);

  l->occupied--;

  printf("[INFO] LOT RELEASED %d/%d BY %s\n", l->occupied, l->capacity, plate);

  // Despertar hilos respetando la prioridad
  if (l->waiting[0] > 0) {
    pthread_cond_signal(&l->cond[0]);
  } else if (l->waiting[1] > 0) {
    pthread_cond_signal(&l->cond[1]);
  } else if (l->waiting[2] > 0) {
    pthread_cond_signal(&l->cond[2]);
  }

  pthread_mutex_unlock(&l->mutex);
}
// ========== FIN SECCIÓN PERSONA B ==========

// ========== SECCIÓN PERSONA C: MAIN Y THREADS ==========
// Arguments for input threads
typedef struct {
  int gate;
  int priority;
  queue *lane_queue;
} ThreadArgs;

int main(int argc, char *argv[]) {

  // AVISO PARA PERSONA C:
  // Recuerda llamar a lot_init(&parking_lot, parking_capacity) aquí
  // antes de lanzar los hilos.

  printf("[OK][parking_manager] Finishing\n");
  return 0;
}