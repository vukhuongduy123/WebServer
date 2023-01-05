#include "httphandle.h"
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

char* fType(char*);
char* responseHeader(int, char*);  // function that builds response header

void httpWorker(int* sockfd /* sockfd contains all the information*/) {
  static const char CONTENTDIR[] = "./contentdir";
  int newsockfd = *sockfd;  // create a local variable for sockfd
  char buffer[256];         // we will read the data in this buffer
  char* token;  // local variable to split the request to get the filename
  bzero(buffer, 256);  // intialize the buffer data to zero
  char fileName[50];
  char homedir[50];
  char* type;
  char* request;
  strcpy(homedir, CONTENTDIR);  // directory where files are stored.
  char* respHeader;             //response header
  // start reading the message from incoming conenction
  int bytesReaded = read(newsockfd, buffer, 255);
  if (bytesReaded < 0) {
    perror("ERROR reading from socket");
    pthread_exit(NULL);
  } else if (bytesReaded == 0) {
    perror("eof");
    pthread_exit(NULL);
  }

  //get the requested file part of the request
  // split string into token seperated by " "
  request = strtok(buffer, " ");
  // in this go we read the file name that needs to be sent
  token = strtok(NULL, " ");
  strcpy(fileName, token);

  // get the complete filename
  if (strcmp(fileName, "/") ==
      0)  // if filename is not provided then we will send index.html
    strcpy(fileName, strcat(homedir, "/index.html"));
  else
    strcpy(fileName, strcat(homedir, fileName));
  type = fType(fileName);  // get file type
  //open file and ready to send
  FILE* fp;
  bool fileExist = true;
  bool staticContent = (strstr(fileName, ".php") == NULL);
  if (staticContent) {
    fp = fopen(fileName, "r");
  } else {
    char fileExecute[256] = "php ";
    strcat(fileExecute, fileName);
    fp = popen(fileExecute, "r");
    if (fp == NULL)
      fp = fopen(fileName, "r");
  }

  if (fp == NULL)
    fileExist = false;

  respHeader = responseHeader(fileExist, type);
  if ((send(newsockfd, respHeader, strlen(respHeader), 0) == -1) ||
      (send(newsockfd, "\r\n", strlen("\r\n"), 0) == -1))
    perror("Failed to send bytes to client");
  // free the allocated memory (note: the memory is allocated in responseheader function)
  free(respHeader);
  
  if (strcmp(request, "HEAD") == 0) {
	  close(newsockfd);
	  return;
  }
  
  if (fileExist) {
    char filechar[1];
    while ((filechar[0] = fgetc(fp)) != EOF) {
      if (send(newsockfd, filechar, sizeof(char), 0) == -1)
        perror("Failed to send bytes to client");
    }
  } else {
    if (send(newsockfd,
             "<html> <HEAD><TITLE>404 Not Found</TITLE></HEAD><BODY>Not "
             "Found</BODY></html> \r\n",
             100, 0) == -1)
      perror("Failed to send bytes to client");
  }

  close(newsockfd);
}

// function below find the file type of the file requested
char* fType(char* fileName) {
  char* type;
  // This returns a pointer to the first occurrence of some character in the string
  char* filetype = strrchr(fileName, '.');
  if ((strcmp(filetype, ".htm")) == 0 || (strcmp(filetype, ".html")) == 0 ||
      strcmp(filetype, ".php") == 0)
    type = "text/html";
  else if ((strcmp(filetype, ".jpg")) == 0)
    type = "image/jpeg";
  else if (strcmp(filetype, ".gif") == 0)
    type = "image/gif";
  else if (strcmp(filetype, ".txt") == 0)
    type = "text/plain";
  else
    type = "application/octet-stream";

  return type;
}

// buildresponseheader
char* responseHeader(int filestatus, char* type) {
  char statuscontent[256] = "HTTP/1.0";
  if (filestatus == 1) {
    strcat(statuscontent, " 200 OK\r\n");
    strcat(statuscontent, "Content-Type: ");
    strcat(statuscontent, type);
    strcat(statuscontent, "\r\n");
  } else {
    strcat(statuscontent, "404 Not Found\r\n");
    // send a blank line to indicate the end of the header lines
    strcat(statuscontent, "Content-Type: ");
    strcat(statuscontent, "NONE\r\n");
  }
  char* returnheader = malloc(strlen(statuscontent) + 1);
  strcpy(returnheader, statuscontent);
  return returnheader;
}

bool isStaticContent(int* socketfd) {
  char buffer[2048] = "";
  if (recv(*socketfd, buffer, 2048, MSG_PEEK) <= 0)
    return NULL;
  static const char* CONTENTDIR = "./contentdir";
  char homedir[50] = "";
  char fileName[50] = "";
  char* token;
  strcpy(homedir, CONTENTDIR);
  token = strtok(buffer, " ");
  token = strtok(NULL, " ");

  strcpy(fileName, token);

  if (strcmp(fileName, "/") == 0) {
    return true;
  }
  strcpy(fileName, strcat(homedir, fileName));
  if (access(fileName, F_OK) != 0) {
    return true;
  }
  if (strstr(fileName, ".php") != NULL) {
    return false;
  }
  return true;
}