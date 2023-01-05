/* A simple server in the internet domain using TCP
   The port number is passed as an argument */
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include "policy.h"
#include "threadpool.h"

#define MAXCONNECTION \
  100  // maximum number of simultaneously connection allowed

void error(const char* msg, int code) {
  perror(msg);
  exit(code);
}

Policy getPolicy(const char* argv) {
  if (strcmp(argv, "ANY") == 0) {
    return ANY;
  } else if (strcmp(argv, "FIFO") == 0) {
    return FIFO;
  } else if (strcmp(argv, "HPSC") == 0) {
    return HPSC;
  } else if (strcmp(argv, "HPDC") == 0) {
    return HPDC;
  }
  error("Policy is not valid", 1);
}

int main(int argc, char* argv[]) {
  Policy policy;
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  struct sockaddr_in serv_addr, cli_addr;
  int n;
  ThreadPool* threadPool;

  if (argc < 4) {
    fprintf(stderr, "ERROR, not enough parameters!!!\n");
    exit(1);
  }
  policy = getPolicy(argv[4]);
  threadPool = createThreadPool(pthread_self(), atoi(argv[2]),atoi(argv[3]), policy);
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket", 2);
  bzero((char*)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR on binding", 3);
  listen(sockfd, MAXCONNECTION);
  while (1) {
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
    if (newsockfd < 0)
      error("ERROR on accept", 4);
    addTask(threadPool, newsockfd);
  }
  close(sockfd);
  return 0;
}

