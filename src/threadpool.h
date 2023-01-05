#ifndef THREAD_POOL_H
#define THREAD_POOL_H
#include <pthread.h>
#include "policy.h"
#include "task.h"
#include "queue.h"

typedef struct {
  void (*httpWorker)(int*);
  int numOfThreads;
  pthread_t* threads;
  pthread_t schedulerThread;
  Task* buffer;
  Queue* requestQueue;
  Task* incommingRequest;
  int bufferSize, counter, out, in;
  pthread_mutex_t* mutex;
  pthread_cond_t *empty, *full, *request;
  Policy policy;
} ThreadPool;

ThreadPool* createThreadPool(pthread_t mainThread, int numOfThreads,
                             int bufferSize, Policy policy);
void addTask(ThreadPool* threadPool, int socketfd);
#endif