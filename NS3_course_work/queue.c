/*
* Author: Anton Belev
* ID: 1103816b
* NS3 Exercise 1
*/

#include "queue.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <pthread.h>

pthread_mutex_t q_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t nonempty_q = PTHREAD_COND_INITIALIZER;


int isEmpty(Queue *q)
{
    return q->size == 0 ? 1 : 0;
}

Queue *queue_create()
{
	Queue *q;
    q = (Queue *)malloc(sizeof(Queue));
    if (q == NULL)
        return NULL;
    q->size = 0;
    q->front = NULL;
    q->rear = NULL;
    return q;
}

Node *node_create(int connfd)
{
	Node *n;
    n = (Node *)malloc(sizeof(Node));
    if (n == NULL)
        return NULL;
    n->next = NULL;
    n->connfd = connfd;
    return n;
}

void enqueue(Queue *q,int connfd)
{
    printf("enqueue connfd =%d...\n", connfd);
    pthread_mutex_lock(&q_mutex);

	Node *n = node_create(connfd);
    if (q->size == 0){
        q->front = n;
        q->rear = n;
    }
    else {
        q->rear->next = n;
    }	
	q->size++;
    printf("q->size =%d...\n", q->size);

    pthread_cond_signal(&nonempty_q);
    pthread_mutex_unlock(&q_mutex);
}

int dequeue(Queue *q)
{
    printf("dequeue 0 q->size =%d...\n", q->size);
    int connfd;
    pthread_mutex_lock(&q_mutex);		
    while (isEmpty(q)){
        pthread_cond_wait(&nonempty_q, &q_mutex);
    }
    printf("IZLEZEEE OT WHILE AAAAAAAAAAAAAA\n");
    pthread_mutex_unlock(&q_mutex);
    printf("SLED UNLOCKA NA AAAAAAAAAAAAAA\n");   
    printf("dequeue 1 q->size =%d...\n", q->size);
    printf("1\n");    
	Node *front = q->front;
    printf("2\n");
    q->front = front->next;
	printf("3\n");
    connfd = front->connfd;
	printf("4\n");
    
    printf("PREDIII FREE FRONT SUUUM\n");
	
    printf("SLEEED FREEE NA FRONT SUUUM\n");    
	q->size--;
    printf("dequeue 2 q->size =%d...\n", q->size);
    printf("dequeue connfd =%d...\n", connfd);
    
    //free(front);
    

	return connfd;
}

static void iterate_nodes_destroy(Node *n)
{
    if (n == NULL)
        return;
    iterate_nodes_destroy(n->next);
    free(n);
}

void queue_destroy(Queue *q)
{
    iterate_nodes_destroy(q->front);
    free(q);
}