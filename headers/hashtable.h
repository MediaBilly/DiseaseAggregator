#ifndef HASHTABLE_H
#define HASHTABLE_H

#include <stdarg.h>
#include "utils.h"

typedef struct hashtable *HashTable;

int HashTable_Create(HashTable*,unsigned int,unsigned int);
int HashTable_Insert(HashTable,string,void*);
void* HashTable_SearchKey(HashTable,string);
void HashTable_ExecuteFunctionForAllKeys(HashTable,void (*func)(string,void*,int,va_list),int, ... );
int HashTable_Destroy(HashTable*,int (*DestroyValueStruct)(void**));

#endif