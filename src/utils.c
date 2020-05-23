#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "../headers/utils.h"

int pipe_size() {
  int p[2];
  if (pipe(p) == -1) {
    perror("pipe");
    exit(1);
  }
  int pipe_size = fpathconf(p[0],_PC_PIPE_BUF);
  close(p[0]);
  close(p[1]);
  return pipe_size;
}

void not_enough_memory() {
  perror("malloc");
}

string CopyString(string str) {
  string ret = NULL;
  if ((ret = (string)malloc(strlen(str) + 1)) != NULL) {
    strcpy(ret,str);
  } else {
    not_enough_memory();
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
char *receive_data(int fd,unsigned int bufferSize,boolean toString) {
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
  if (total_read == 0) {
    return NULL;
  }
  // Set \0 for string ending if needed
  if (toString) {
    bytestring = realloc(bytestring,total_read + 1);
    bytestring[total_read] = 0;
  }
  return bytestring;
}

unsigned int wordCount(string str) {
  unsigned int count = 0;
  // Ignore 1st potential gaps
  while (*str == ' ') str++;
  while (*str != '\n' && *str != 0) {
    count++;
    // Loop through characters
    while (*str != ' ' && *str != '\n' && *str != 0) str++;
    // Ignore gaps
    while (*str == ' ') str++;
  }
  return count;
}

string IgnoreNewLine(string str) {
  str[strlen(str) - 1] = 0;
  return str;
}

string* SplitString(string str,string delimeter) {
  string *array;
  if ((array = (string*)malloc(wordCount(str)*sizeof(string))) == NULL) {
    not_enough_memory();
    return NULL;
  }
  unsigned int index = 0;
  string tmp = strtok(str,delimeter);
  while (tmp != NULL) {
    array[index++] = tmp;
    tmp = strtok(NULL," ");
  }
  return array;
}