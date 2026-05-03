
#include "queue.h"



//To create a queue
queue* queue_init(int size){

    queue * q = (queue *)malloc(sizeof(queue));

    return q;
}


// To Enqueue a car
int queue_put(queue *q, struct car* x) {
    return 0;
}


// To Dequeue an car.
struct car* queue_get(queue *q) {
    struct car* element;
    
    return element;
}


//To check queue state
int queue_empty(queue *q){
    
    return 0;
}

int queue_full(queue *q){
    
    return 0;
}

//To destroy the queue and free the resources
int queue_destroy(queue *q){
    return 0;
}
