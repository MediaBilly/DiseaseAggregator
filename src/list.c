#include <stdlib.h>
#include <string.h>
#include "../headers/list.h"
#include "../headers/utils.h"

typedef struct listnode *ListNode;

struct listnode {
  string value;
  ListNode next;
};

struct list {
  ListNode head;
  ListNode tail;
};

int List_Initialize(List *list) {
  // Allocate memory for list structure
  if ((*list = (List)malloc(sizeof(struct list))) == NULL) {
    not_enough_memory();
    *list = NULL;
    return FALSE;
  }
  (*list)->head = NULL;
  (*list)->tail = NULL;
  return TRUE;
}

int List_Insert(List list,string value) {
  // Allicate memory for new node
  ListNode tmp;
  if ((tmp = (ListNode)malloc(sizeof(struct listnode))) == NULL) {
    not_enough_memory();
    return FALSE;
  }
  // Initialize new node's data members
  tmp->next = NULL;
  tmp->value = CopyString(value);
  // Insert at tail(last)
  if (list->head != NULL) {
    // Not 1st element
    list->tail->next = tmp;
    list->tail = list->tail->next;
  } else {
    // 1st element
    list->head = list->tail = tmp;
  }
  return TRUE;
}

int List_Destroy(List *list) {
  // Check if list was previously initialized
  if (*list != NULL) {
    ListNode toDel;
    // Loop through all the nodes and delete them
    while ((*list)->head != NULL) {
      toDel = (*list)->head;
      // Delete value's memory
      DestroyString(&(toDel->value));
      // Point to the next node
      (*list)->head = (*list)->head->next;
      // Delete node's memory
      free(toDel);
    }
    // Free list's memory
    free(*list);
    *list = NULL;
    return TRUE;
  } else {
    return FALSE;
  }
}

ListIterator List_CreateIterator(List list) {
  return list != NULL ? list->head : NULL;
}

void ListIterator_MoveToNext(ListIterator *it) {
  *it = (*it)->next;
}

string ListIterator_GetValue(ListIterator it) {
  return it->value;
}