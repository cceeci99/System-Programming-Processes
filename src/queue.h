#ifndef QUEUE_H
#define QUEUE_H


// ------------ Implementation of Queue Struct using linked list -------------- //


struct queue_node {             // a queue node stores the pid of process
    pid_t pid;
    struct queue_node* next;    // pointer to next node
};

typedef struct queue_node* queue_node;


struct queue {                  // queue has a front and tail node (circular queue)
    queue_node front;
    queue_node tail;
};

typedef struct queue* queue;


// ------------- Queue Methods -------------- //


/*---- Create queue ----*/
queue create_queue();


/*---- Delete queue ----*/
void delete_queue(queue q);


/*---- Create new queue node given pid p----*/
queue_node new_node(pid_t p);


/*---- Add pid to the queue ----*/
void enqueue(queue q, pid_t p);


/*---- Remove node from queue and return pid----*/
pid_t dequeue(queue q);


/*---- Check if a queue is empty ----*/
int empty(queue q);


#endif // QUEUE_H