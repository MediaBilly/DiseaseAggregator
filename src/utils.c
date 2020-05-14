#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "../headers/utils.h"

void not_enough_memory() {
  perror("malloc");
}

string CopyString(string str) {
  string ret = NULL;
  if ((ret = (string)malloc(strlen(str) + 1)) != NULL) {
    strcpy(ret,str);
  }
  return ret;
}

int DestroyString(string *str) {
  if (*str != NULL) {
    free(*str);
    *str = NULL;
    return TRUE;
  } else {
    return FALSE;
  }
}

// Function to send data to a pipe
void send_data(int fd,char *data,unsigned int dataSize,unsigned int bufferSize) {
  unsigned int remBytes = dataSize;
  // Send first chunks with specified bufferSize
  while (remBytes >= bufferSize && write(fd,data,bufferSize) > 0) {
    remBytes -= bufferSize;
    data += bufferSize;
  }
  // Send last chunk with size < bufferSize
  if (remBytes > 0) {
    write(fd,data,remBytes);
  }
}

// Function that reads data from a pipe and returns it to a dynamically allocated array
char *receive_data(int fd,unsigned int bufferSize) {
  ssize_t bytes_read = 0;
  char buffer[bufferSize];
  char *tmp,*bytestring = NULL;
  unsigned int total_read = 0;
  // Read chunks with specified bufferSize
  while ((bytes_read = read(fd,buffer,bufferSize)) > 0) {
    total_read += bytes_read;
    if ((tmp = realloc(bytestring,total_read)) == NULL) {
      not_enough_memory();
      return NULL;
    } else {
      bytestring = tmp;
    }
    memcpy(bytestring + total_read - bytes_read,buffer,bytes_read);
  }
  return bytestring;
}