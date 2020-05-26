#ifndef UTILITIES_H
#define UTILITIES_H

typedef char* string;

#define CEIL(a,b) (((a)+(b)-1)/(b))

#define FIFO_PERMS 0666

typedef int boolean;

#define TRUE 1
#define FALSE 0

int pipe_size();
void not_enough_memory();
string CopyString(string);
int DestroyString(string*);
void send_data(int,char*,unsigned int,unsigned int);
char *receive_data(int,unsigned int,boolean);
// Counts # of words in a string
unsigned int wordCount(string);
string IgnoreNewLine(string);
string* SplitString(string,string);
int digits(unsigned int);

#endif