#ifndef UTILITIES_H
#define UTILITIES_H

typedef char* string;

#define CEIL(a,b) (((a)+(b)-1)/(b))

#define FIFO_PERMS 0666

#define TRUE 1
#define FALSE 0

void not_enough_memory();
string CopyString(string);
int DestroyString(string*);

#endif