#ifndef TASK_H
#define TASK_H
typedef struct {
  int socketfd;
  int priority;
} Task;
#endif