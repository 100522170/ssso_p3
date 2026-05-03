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

// Estructura parking
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

//  MAIN Y THREADS

// Sincronización global para la señal de inicio del parking_manager
pthread_mutex_t start_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t start_cond = PTHREAD_COND_INITIALIZER;
int start_flag = 0;

// Arrays de Colas para entrada y salida
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

// Función de validación de matrícula (ej: 1111-AAA)
int is_valid_plate(const char *plate) {
  if (strlen(plate) != 8)
    return 0;
  for (int i = 0; i < 4; i++)
    if (!isdigit(plate[i]))
      return 0;
  if (plate[4] != '-')
    return 0;
  for (int i = 5; i < 8; i++)
    if (!isupper(plate[i]))
      return 0;
  return 1;
}

// Hilo Gestor de Entrada
void *entry_lane_thread(void *arg) {
  ThreadArgs *args = (ThreadArgs *)arg;
  if (!args) {
    fprintf(stderr, "[ERROR] [entry_lane_thread] Arguments not valid\n");
    return (void *)-1;
  }

  int var_lane = args->priority;
  int var_gate = args->gate;

  printf("[LANE %d] [GATE %d] Waiting\n", var_lane, var_gate);

  // Bloqueo hasta recibir la señal
  pthread_mutex_lock(&start_mutex);
  while (start_flag == 0) {
    pthread_cond_wait(&start_cond, &start_mutex);
  }
  pthread_mutex_unlock(&start_mutex);

  while (1) {
    // AHORA SACAMOS UN STRUCT CAR, NO UN CHAR*
    struct car *c = (struct car *)queue_get(args->lane_queue);

    // Centinela "END"
    if (c == NULL || strcmp(c->plate, "END") == 0) {
      if (c != NULL)
        free(c);
      break;
    }

    printf("[LANE %d] [GATE %d] Car %s\n", var_lane, var_gate, c->plate);

    lot_claim_space(&parking_lot, var_lane, c->plate);

    int exit_gate = var_gate % NUM_EXIT_GATES;
    // pasamos la struct a exit y como ya no se usa, la liberamos con free
    queue_put(exit_queues[exit_gate], c);
    free(c);
  }

  printf("[LANE %d] [GATE %d] Ending\n", var_lane, var_gate);
  free(args);
  pthread_exit((void *)0);
}

// Hilo Gestor de Salida
void *exit_gate_thread(void *arg) {
  ExitThreadArgs *args = (ExitThreadArgs *)arg;
  if (!args) {
    fprintf(stderr, "[ERROR] [EXIT -1] Arguments not valid\n");
    return (void *)-1;
  }

  int exit_gate = args->gate;

  while (1) {
    // AHORA SACAMOS UN STRUCT CAR
    struct car *c = (struct car *)queue_get(args->exit_queue);

    if (c == NULL || strcmp(c->plate, "END") == 0) {
      if (c != NULL)
        free(c);
      break;
    }

    printf("[EXIT %d] CAR %s\n", exit_gate, c->plate);

    lot_release_space(&parking_lot, c->plate);
    // la memoria no se necesita asi que se libera
    free(c);
  }

  printf("[EXIT %d] Ending\n", exit_gate);
  free(args);
  pthread_exit((void *)0);
}

int main(int argc, char *argv[]) {
  // Comprobación de argumentos
  if (argc != 4) {
    fprintf(stderr, "Usage: %s <input file> <queue size> <parking capacity>\n",
            argv[0]);
    return -1;
  }

  int queue_size = atoi(argv[2]);
  int parking_capacity = atoi(argv[3]);

  if (queue_size <= 0) {
    fprintf(stderr, "Error: queue size incorect\n");
    return -1;
  }
  if (parking_capacity <= 0) {
    fprintf(stderr, "Error: parking capacity incorrect\n");
    return -1;
  }

  // AVISO PERSONA C: Iniciamos la estructura Lot de la Persona B
  lot_init(&parking_lot, parking_capacity);

  // Inicializar colas circulares de entrada y salida
  for (int i = 0; i < NUM_GATES; i++) {
    for (int j = 0; j < NUM_PRIORITIES; j++) {
      entry_queues[i][j] = queue_init(queue_size);
    }
  }
  for (int i = 0; i < NUM_EXIT_GATES; i++) {
    exit_queues[i] = queue_init(queue_size);
  }

  // Lanzar hilos de entrada
  pthread_t entry_tids[NUM_GATES][NUM_PRIORITIES];
  for (int i = 0; i < NUM_GATES; i++) {
    for (int j = 0; j < NUM_PRIORITIES; j++) {
      ThreadArgs *args = malloc(sizeof(ThreadArgs));
      args->gate = i;
      args->priority = j;
      args->lane_queue = entry_queues[i][j];

      if (pthread_create(&entry_tids[i][j], NULL, entry_lane_thread, args) !=
          0) {
        perror("[ERROR] Creating entry thread");
        return -1;
      }
    }
  }

  // Lanzar hilos de salida
  pthread_t exit_tids[NUM_EXIT_GATES];
  for (int i = 0; i < NUM_EXIT_GATES; i++) {
    ExitThreadArgs *args = malloc(sizeof(ExitThreadArgs));
    args->gate = i;
    args->exit_queue = exit_queues[i];

    if (pthread_create(&exit_tids[i], NULL, exit_gate_thread, args) != 0) {
      perror("[ERROR] Creating exit thread");
      return -1;
    }
  }

  // Parsear fichero CSV
  int fd = open(argv[1], O_RDONLY);
  if (fd == -1) {
    perror("[ERROR] [parking_manager] Opening file");
    return -1;
  }

  // VECTOR DE ESTRUCTURAS
  struct car *car_vector =
      malloc(1024 * sizeof(struct car)); // Capacidad máxima asumida
  int car_count = 0;

  char c;
  ssize_t bytes_read;
  char line[256];
  int pos = 0;
  int is_first_line = 1;

  while ((bytes_read = read(fd, &c, 1)) > 0) {
    if (c == '\n' || c == '\r') {
      if (pos > 0) {
        line[pos] = '\0';
        pos = 0;

        if (is_first_line) {
          is_first_line = 0;
          continue;
        }

        char plate_buf[16];
        int gate_buf, priority_buf;

        if (sscanf(line, " %15[^,], %d , %d", plate_buf, &gate_buf,
                   &priority_buf) == 3) {
          int len = (int)strlen(plate_buf);
          while (len > 0 && isspace(plate_buf[len - 1])) {
            plate_buf[len - 1] = '\0';
            len--;
          }

          if (is_valid_plate(plate_buf) && gate_buf >= 0 &&
              gate_buf < NUM_GATES && priority_buf >= 0 &&
              priority_buf < NUM_PRIORITIES) {
            // guardamos en el vector de estruturas
            if (car_count >= 1024) {
              fprintf(stderr, "Error: too many cars)\n");
              continue;
            }
            strcpy(car_vector[car_count].plate, plate_buf);
            car_vector[car_count].gate = gate_buf;
            car_vector[car_count].priority = priority_buf;
            car_count++;
          }
        }
      }
    } else {
      if (pos < 255)
        line[pos++] = c;
    }
  }

  if (bytes_read < 0) {
    perror("[ERROR] [parking_manager] Reading file");
    close(fd);
    return -1;
  }

  if (close(fd) == -1) {
    perror("[ERROR] [parking_manager] Closing file");
    return -1;
  }

  // señaliza a entry para empezar
  pthread_mutex_lock(&start_mutex);
  start_flag = 1;
  pthread_cond_broadcast(&start_cond);
  pthread_mutex_unlock(&start_mutex);

  // llenar la queue con coches
  for (int i = 0; i < car_count; i++) {
    queue_put(entry_queues[car_vector[i].gate][car_vector[i].priority],
              &car_vector[i]);
  }

  // Mandar el "centinela END" como struct
  for (int i = 0; i < NUM_GATES; i++) {
    for (int j = 0; j < NUM_PRIORITIES; j++) {
      struct car *end_car = malloc(sizeof(struct car));
      strcpy(end_car->plate, "END");
      queue_put(entry_queues[i][j], end_car);
    }
  }

  // esperamos a que entry termine de procesar los coches
  for (int i = 0; i < NUM_GATES; i++) {
    for (int j = 0; j < NUM_PRIORITIES; j++) {
      void *ret;
      pthread_join(entry_tids[i][j], &ret);
      if (ret == (void *)-1) {
        fprintf(
            stderr,
            "[ERROR][parking_manager] LANE %d of GATE %d FINISHED WITH ERROR\n",
            j, i);
      }
    }
  }

  // los coches ya han sido procesados, ahora mandamos end a exit
  for (int i = 0; i < NUM_EXIT_GATES; i++) {
    struct car *end_car = malloc(sizeof(struct car));
    strcpy(end_car->plate, "END");
    queue_put(exit_queues[i], end_car);
  }

  // Esperar a que terminen de salir
  for (int i = 0; i < NUM_EXIT_GATES; i++) {
    void *ret;
    pthread_join(exit_tids[i], &ret);
    if (ret == (void *)-1) {
      fprintf(stderr,
              "[ERROR] [parking_manager] EXIT GATE %d FINISHED WITH ERROR\n",
              i);
    }
  }

  // librerar recursos
  for (int i = 0; i < NUM_GATES; i++) {
    for (int j = 0; j < NUM_PRIORITIES; j++) {
      queue_destroy(entry_queues[i][j]);
    }
  }
  for (int i = 0; i < NUM_EXIT_GATES; i++) {
    queue_destroy(exit_queues[i]);
  }

  // hacemos el free para el vector de coches
  free(car_vector);

  // destruimos las sincronizacionesDestroy global synchronization primitives
  pthread_mutex_destroy(&start_mutex);
  pthread_cond_destroy(&start_cond);

  printf("[OK][parking_manager] Finishing\n");
  return 0;
}
