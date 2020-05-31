#ifndef AVLTREE_H
#define AVLTREE_H

#include <time.h>
#include "patientRecord.h"
#include "utils.h"

typedef struct avltree *AvlTree;

int AvlTree_Create(AvlTree*);
int AvlTree_Insert(AvlTree,patientRecord);
unsigned int AvlTree_NumRecords(AvlTree);
unsigned int AvlTree_NumRecordsInDateRange(AvlTree,time_t,time_t,boolean);
void AvlTree_topk_Age_Ranges(AvlTree,time_t,time_t,unsigned int,int,unsigned int);
int AvlTree_Destroy(AvlTree*);

#endif