/*
* Author: Anton Belev
* ID: 1103816b
* NS3 Exercise 1
*/

#ifndef _QUEUE_H_INCLUDED_
#define _QUEUE_H_INCLUDED_
#include <pthread.h>

typedef struct queue Queue;
typedef struct node Node;

struct queue{
	pthread_mutex_t q_mutex;
	pthread_cond_t nonempty_q;
    Node *front;
    Node *rear;
    long size;
};

struct node{
    Node *next;
    int connfd;
};

int isEmpty(Queue *q);
    
Queue *queue_create();

Node *node_create(int connfd);

void enqueue(Queue *q,int connfd);

int dequeue(Queue *q);

void queue_destroy(Queue *q);

#endif /* _QUEUE_H_INCLUDED_ */