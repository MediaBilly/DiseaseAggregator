#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "../headers/hashtable.h"
#include "../headers/utils.h"

struct hashtable {
  unsigned int numOfEntries;
  unsigned int bucketSize;
  unsigned int totalRecords;
  char **entries;
};

int HashTable_Create(HashTable *table,unsigned int numOfEntries,unsigned int bucketSize) {
  // Allocate memory for HashTable data structure
  if ((*table = (HashTable)malloc(sizeof(struct hashtable))) == NULL) {
    not_enough_memory();
    return FALSE;
  }
  // Allocate memory for entries table
  if (((*table)->entries = (char**)malloc(numOfEntries * sizeof(char*))) == NULL) {
    not_enough_memory();
    free(*table);
    return FALSE;
  }
  // Initialize attributes
  (*table)->numOfEntries = numOfEntries;
  (*table)->bucketSize = bucketSize;
  (*table)->totalRecords = 0;
  unsigned int i;
  for (i = 0; i < numOfEntries; i++) {
    (*table)->entries[i] = NULL;
  }
  return TRUE;
}

unsigned int hash(char *str,unsigned int size) {
    // Compute str key
    int i;
    unsigned int k = 0,l = strlen(str),base = 1;
    for (i = l - 1;i >= 0;i--) {
        k = (k % size + (base * (str[i] % size)) % size) % size;
        base = (base * (128 % size)) % size;
    }
    // Return global hash
    return k;
}

char *CreateBucket(unsigned int bucketSize) {
  char *bucketData;
  if ((bucketData = (char*)malloc(bucketSize * sizeof(char))) == NULL) {
    return NULL;
  }
  memset(bucketData,0,bucketSize);
  return bucketData;
}

int Bucket_InsertRecord(char *bucketData,unsigned int bucketSize,string key,void *value) {
  size_t recordSize = sizeof(string) + sizeof(void*);
  unsigned int offset;
  for (offset = 0;offset <= bucketSize - sizeof(void*) - recordSize;offset += recordSize) {
    string currentKey;
    memcpy(&currentKey,bucketData + offset,sizeof(string));
    // We found an available position to store the new record
    if (currentKey == NULL) {
      // Insert key (string pointer)
      string keyCopy = CopyString(key);
      memcpy(bucketData + offset,&keyCopy,sizeof(string));
      // Insert value
      memcpy(bucketData + offset + sizeof(string),&value,sizeof(void*));
      return TRUE;
    }
  }
  // We reached the end of the bucket and the new record does not fit
  return FALSE;
}

void* Bucket_SearchKey(char *bucketData,unsigned int bucketSize,string key) {
  size_t recordSize = sizeof(string) + sizeof(void*);
  unsigned int offset = 0;
  for (offset = 0;offset <= bucketSize - sizeof(void*) - recordSize;offset += recordSize) {
    string currentKey;
    memcpy(&currentKey,bucketData + offset,sizeof(string));
    // Found
    if (currentKey != NULL) {
      if(!strcmp(currentKey,key)) {
        char *retValue;
        memcpy(&retValue,bucketData + offset + sizeof(string),sizeof(void*));
        return retValue;
      }
    } else {
      return NULL;
    }
  }
  // We reached the end of the bucket and the new record does not fit
  return NULL;
}

void Bucket_Destroy(char *bucketData,unsigned int bucketSize,int (*DestroyValueStruct)(void**)) {
  size_t recordSize = sizeof(string) + sizeof(void*);
  unsigned int offset = 0;
  // Iterate through all bucket's records
  for (offset = 0;offset <= bucketSize - sizeof(void*) - recordSize;offset += recordSize) {
    string currentKey;
    memcpy(&currentKey,bucketData + offset,sizeof(string));
    // Destroy Key
    DestroyString(&currentKey);
    // Destroy value
    void *currentValue;
    memcpy(&currentValue,bucketData + offset + sizeof(string),sizeof(void*));
    if (DestroyValueStruct != NULL) {
      (*DestroyValueStruct)(&currentValue);
    }
  }
  free(bucketData);
}

// Creates a new bucket and makes the given one point to the new one
int Bucket_CreateNext(char *lastBucketData,unsigned int bucketSize) {
  // Create new bucket
  char *newBucketData;
  if ((newBucketData = CreateBucket(bucketSize)) == NULL) {
    return FALSE;
  }
  // Copy pointer for the new bucket to the one given
  memcpy(lastBucketData + bucketSize - sizeof(char*),&newBucketData,sizeof(char*));
  return TRUE;
}

char* Bucket_Next(char *bucketData,unsigned int bucketSize) {
  char *ret;
  memcpy(&ret,bucketData + bucketSize - sizeof(char*),sizeof(char*));
  return ret;
}

int HashTable_Insert(HashTable table,string key,void *value) {
  unsigned int entry = hash(key,table->numOfEntries);
  // First record for this entry
  if (table->entries[entry] == NULL) {
    char *newBucket;
    if ((newBucket = CreateBucket(table->bucketSize)) == NULL) {
      not_enough_memory();
      return FALSE;
    }
    Bucket_InsertRecord(newBucket,table->bucketSize,key,value);
    table->entries[entry] = newBucket;
    return TRUE;
  } else {
    // Not 1st record so iterate through the buckets until we find an available place
    char *currentBucket = table->entries[entry];
    char *previousBucket = NULL;
    while (currentBucket != NULL) {
      if (Bucket_InsertRecord(currentBucket,table->bucketSize,key,value)) {
        // Successful insertion in the current bucket
        return TRUE;
      } else {
        // Otherwise move to the next bucket keeping pointer to the previous one
        previousBucket = currentBucket;
        currentBucket = Bucket_Next(currentBucket,table->bucketSize);
      }
    }
    // No available space in current buckets so create a new one and insert the record there
    if (Bucket_CreateNext(previousBucket,table->bucketSize)) {
      currentBucket = Bucket_Next(previousBucket,table->bucketSize);
      return Bucket_InsertRecord(currentBucket,table->bucketSize,key,value);
    } else {
      return FALSE;
    }
  }
}

void* HashTable_SearchKey(HashTable table,string key) {
  unsigned int entry = hash(key,table->numOfEntries);
  if (table->entries[entry] != NULL) {
    char *currentBucket = table->entries[entry];
    while (currentBucket != NULL) {
      void *currentValue;
      if ((currentValue = Bucket_SearchKey(currentBucket,table->bucketSize,key)) != NULL) {
        // Successful insertion in the current bucket
        return currentValue;
      } else {
        // Otherwise move to the next bucket
        currentBucket = Bucket_Next(currentBucket,table->bucketSize);
      }
    }
    // Not found
    return NULL;
  } else {
    // Not found(no bucket in current entry)
    return NULL;
  }
}

void HashTable_ExecuteFunctionForAllKeys(HashTable table,void (*func)(string,void*,int,va_list),int argc, ... ) {
  if (table != NULL) {
    unsigned int curEntry;
    // Iterate through all the entries
    for (curEntry = 0; curEntry < table->numOfEntries; curEntry++) {
      char *currentBucket = table->entries[curEntry];
      // Iterate through all the entry's buckets
      while (currentBucket != NULL) {
        char *nextBucket = Bucket_Next(currentBucket,table->bucketSize);
        size_t recordSize = sizeof(string) + sizeof(void*);
        unsigned int offset = 0;
        // Iterate through all bucket's records
        for (offset = 0;offset <= table->bucketSize - sizeof(void*) - recordSize;offset += recordSize) {
          va_list valist;
          va_start(valist,argc);
          // Get key
          string currentKey;
          memcpy(&currentKey,currentBucket + offset,sizeof(string));
          if (currentKey != NULL) {
            // Get value
            void *currentValue;
            memcpy(&currentValue,currentBucket + offset + sizeof(string),sizeof(void*));
            (*func)(currentKey,currentValue,argc,valist);
            va_end(valist);
          } else {
            va_end(valist);
            break;
          }
        }
        currentBucket = nextBucket;
      }
    }
  } 
}

int HashTable_Destroy(HashTable *table,int (*DestroyValueStruct)(void**)) {
  if (*table != NULL) {
    unsigned int curEntry;
    // Destroy all buckets including their data
    for (curEntry = 0; curEntry < (*table)->numOfEntries; curEntry++) {
      char *currentBucket = (*table)->entries[curEntry];
      while (currentBucket != NULL) {
        char *nextBucket = Bucket_Next(currentBucket,(*table)->bucketSize);
        Bucket_Destroy(currentBucket,(*table)->bucketSize,DestroyValueStruct);
        currentBucket = nextBucket;
      }
    }
    // Destroy entries table
    free((*table)->entries);
    free(*table);
    *table = NULL;
    return TRUE;
  } else {
    return FALSE;
  }
}
