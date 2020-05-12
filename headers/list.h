#ifndef LIST_H
#define LIST_H

#include "utils.h"

// Define list ADT
typedef struct list *List;

// Defile list iterator type
typedef struct listnode *ListIterator;

// Define list functions
int List_Initialize(List*);
int List_Insert(List,string);
int List_Destroy(List*);

// Define list iterator functions
ListIterator List_CreateIterator(List);
void ListIterator_MoveToNext(ListIterator*);
string ListIterator_GetValue(ListIterator);

#endif