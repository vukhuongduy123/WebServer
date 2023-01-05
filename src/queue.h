#ifndef QUEUE_H
#define QUEUE_H
#include <stdbool.h>

typedef struct {
  int front, rear, size;
  unsigned capacity;
  int* array;
} Queue;

Queue* createQueue(unsigned capacity);
bool isFull(Queue* queue);
bool isEmpty(Queue* queue);
void enqueue(Queue* queue, int item);
int dequeue(Queue* queue);
int front(Queue* queue);
int rear(Queue* queue);
#endif  // ! QUEUE_H
