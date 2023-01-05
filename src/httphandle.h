#ifndef HTTP_HANDLE
#define HTTP_HANDLE
#include <stdbool.h>
void httpWorker(int*);  // This function will handle request
bool isStaticContent(int* socket);

#endif  // !HTTP_HANDLE
