#include "threadpool.h"
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "httphandle.h"
#include "task.h"

void* produce(void* args);
void* consume(void* args);
void sortByPriority(ThreadPool* threadPool);

ThreadPool* createThreadPool(pthread_t mainThread, int numOfThreads,
                             int bufSize, Policy policy) {
  ThreadPool* threadPool = malloc(sizeof(ThreadPool));
  threadPool->bufferSize = bufSize;
  threadPool->buffer = malloc(sizeof(Task) * threadPool->bufferSize);
  for (int i = 0; i < threadPool->bufferSize; i++) {
    threadPool->buffer[i].socketfd = 0;
    threadPool->buffer[i].priority = INT_MIN;
  }
  threadPool->httpWorker = httpWorker;
  threadPool->numOfThreads = numOfThreads + 2;
  threadPool->requestQueue = createQueue(100);
  threadPool->counter = 0;
  threadPool->out = 0;
  threadPool->in = 0;
  threadPool->policy = policy;

  threadPool->mutex = malloc(sizeof(pthread_mutex_t));
  pthread_mutex_init(threadPool->mutex, NULL);
  threadPool->empty = malloc(sizeof(pthread_cond_t));
  pthread_cond_init(threadPool->empty, NULL);
  threadPool->full = malloc(sizeof(pthread_cond_t));
  pthread_cond_init(threadPool->full, NULL);
  threadPool->request = malloc(sizeof(pthread_cond_t));
  pthread_cond_init(threadPool->request, NULL);

  threadPool->threads = malloc(sizeof(pthread_t) * numOfThreads);
  pthread_t thread;
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_create(&threadPool->schedulerThread, NULL, produce, threadPool);
  for (int i = 0; i < numOfThreads; i++) {
    pthread_create(&thread, &attr, consume, threadPool);
    threadPool->threads[i] = thread;
  }

  return threadPool;
}

void* produce(void* args) {
  while (true) {
    ThreadPool* threadPool = args;
    pthread_mutex_lock(threadPool->mutex);

    while (isEmpty(threadPool->requestQueue)) {
      pthread_cond_wait(threadPool->request, threadPool->mutex);
    }

    while (threadPool->counter == threadPool->bufferSize) {
      pthread_cond_wait(threadPool->empty, threadPool->mutex);
    }

    threadPool->incommingRequest = malloc(sizeof(Task));
    threadPool->incommingRequest->socketfd = dequeue(threadPool->requestQueue);
    if (threadPool->policy == ANY || threadPool->policy == FIFO) {
      threadPool->incommingRequest->priority = 1;
    } else {
      bool staticContent =
          isStaticContent(&threadPool->incommingRequest->socketfd);
      if (threadPool->policy == HPSC) {
        threadPool->incommingRequest->priority = (staticContent == true) ? 2 : 1;
      } else if (threadPool->policy == HPDC) {
        threadPool->incommingRequest->priority = (staticContent == true) ? 1 : 2;
      }
    }


    threadPool->buffer[threadPool->out] = *threadPool->incommingRequest;
    sortByPriority(threadPool);
    threadPool->out = (threadPool->out + 1) % threadPool->bufferSize;
    threadPool->counter++;
    free(threadPool->incommingRequest);
    threadPool->incommingRequest = NULL;
    pthread_cond_signal(threadPool->full);
    pthread_mutex_unlock(threadPool->mutex);
  }
}

void* consume(void* args) {
  while (true) {
    ThreadPool* threadPool = args;
    pthread_mutex_lock(threadPool->mutex);
    while (threadPool->counter == 0) {
      pthread_cond_wait(threadPool->full, threadPool->mutex);
    }
    Task* consumed = malloc(sizeof(Task));
    consumed->socketfd = threadPool->buffer[threadPool->in].socketfd;
    threadPool->in = (threadPool->in + 1) % threadPool->bufferSize;
    threadPool->counter--;
    pthread_cond_signal(threadPool->empty);
    pthread_mutex_unlock(threadPool->mutex);
    httpWorker(&consumed->socketfd);
    free(consumed);
  }
}

void addTask(ThreadPool* threadPool, int socketfd) {
  pthread_mutex_lock(threadPool->mutex);
  enqueue(threadPool->requestQueue, socketfd);
  pthread_cond_signal(threadPool->request);
  pthread_mutex_unlock(threadPool->mutex);
}

void sortByPriority(ThreadPool* threadPool) {
  int o = threadPool->out, i = threadPool->in;
  while (o != i) {
    if (threadPool->buffer[i].priority <
        threadPool->incommingRequest->priority) {
      break;
    }
    i = (i + 1) % threadPool->bufferSize;
  }

  Task temp = threadPool->buffer[i];
  threadPool->buffer[i] = threadPool->buffer[threadPool->out];
  threadPool->buffer[threadPool->out] = temp;
}
