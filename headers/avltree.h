#ifndef AVLTREE_H
#define AVLTREE_H

#include <time.h>
#include "patientRecord.h"

typedef struct avltree *AvlTree;

int AvlTree_Create(AvlTree*);
int AvlTree_Insert(AvlTree,patientRecord);
unsigned int AvlTree_NumRecords(AvlTree);
unsigned int AvlTree_NumRecordsInDateRange(AvlTree,time_t,time_t);
int AvlTree_Destroy(AvlTree*);

#endif