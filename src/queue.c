#include <stdio.h>
#include <stdlib.h>

#include "queue.h"
#include "utils.h"


queue create_queue() {

	queue q = malloc(sizeof(queue));
	if (q == NULL) {
		perror_exit("malloc");
	}

	q->front = NULL;
    q->tail = NULL;

	return q;
}


void delete_queue(queue q) {

	if (q->front == NULL) {
		return;
	}

	queue_node temp = q->front;

	while (temp != NULL) {
		dequeue(q);
		temp = temp->next;
	}

	free(q);

	q->front = NULL;
	q->tail = NULL;
}


queue_node new_node(pid_t p) {

	queue_node node = malloc(sizeof(queue_node));
	if (node == NULL) {
		perror_exit("malloc");
	}

	node->pid = p;
	node->next = NULL;

	return node;
}


void enqueue(queue q, pid_t p) {
	
	queue_node node = new_node(p);

	if (q->front == NULL) {
		q->front = node;
        q->tail = node;

		return;
	}

	q->tail->next = node;
	q->tail = node;
}


pid_t dequeue(queue q) {

	if (q->front == NULL) {
		return -1;
	}

	queue_node temp = q->front;
	q->front = q->front->next;

	if (q->front == NULL) {
		q->tail = NULL;
	}

	pid_t pid = temp->pid;

	free(temp);

	return pid;
}


int empty(queue q) {
	return q->front == NULL && q->tail == NULL;
}