#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "../headers/hashtable.h"
#include "../headers/utils.h"

struct hashtable {
  unsigned int numOfEntries;
  unsigned int totalRecords;
  char **entries;
};

#define BUCKET_SIZE (sizeof(string) + 2*sizeof(void*))

// Same implementation as project 1 but for simplicity this time bucket size is always sizeof(string) + 2*sizeof(void*) 
// (fits only one record and a pointer to the next bucket)

int HashTable_Create(HashTable *table,unsigned int numOfEntries) {
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

char *CreateBucket() {
  char *bucketData;
  if ((bucketData = (char*)malloc(BUCKET_SIZE)) == NULL) {
    return NULL;
  }
  memset(bucketData,0,BUCKET_SIZE);
  return bucketData;
}

int Bucket_InsertRecord(char *bucketData,string key,void *value) {
  string currentKey;
  memcpy(&currentKey,bucketData,sizeof(string));
  // We found an available position to store the new record
  if (currentKey == NULL) {
    // Insert key (string pointer)
    string keyCopy;
    if ((keyCopy = CopyString(key)) == NULL) {
      return FALSE;
    }
    memcpy(bucketData,&keyCopy,sizeof(string));
    // Insert value
    memcpy(bucketData + sizeof(string),&value,sizeof(void*));
    return TRUE;
  }
  // We reached the end of the bucket and the new record does not fit
  return FALSE;
}

void* Bucket_SearchKey(char *bucketData,string key) {
  string currentKey;
  memcpy(&currentKey,bucketData,sizeof(string));
  // Found
  if (currentKey != NULL) {
    if(!strcmp(currentKey,key)) {
      char *retValue;
      memcpy(&retValue,bucketData + sizeof(string),sizeof(void*));
      return retValue;
    }
  } else {
    return NULL;
  }
  // We reached the end of the bucket and the wanted key was not found
  return NULL;
}

int Bucket_ReplaceKey(char *bucketData,string key,int (*DestroyValueStruct)(void**),void *newValue) {
  string currentKey;
  memcpy(&currentKey,bucketData,sizeof(string));
  // Found
  if (currentKey != NULL) {
    if(!strcmp(currentKey,key)) {
      // Get previous value
      void *value;
      memcpy(&value,bucketData + sizeof(string),sizeof(void*));
      // Destroy previous value
      if (DestroyValueStruct != NULL) {
        (*DestroyValueStruct)(&value);
      }
      // Insert new value
      memcpy(bucketData + sizeof(string),&newValue,sizeof(void*));
      return TRUE;
    }
  } else {
    return FALSE;
  }
  // We reached the end of the bucket and the wanted key was not found
  return FALSE;
}

void Bucket_Destroy(char *bucketData,int (*DestroyValueStruct)(void**)) {
  string currentKey;
  memcpy(&currentKey,bucketData,sizeof(string));
  // Destroy Key
  DestroyString(&currentKey);
  // Destroy value
  void *currentValue;
  memcpy(&currentValue,bucketData + sizeof(string),sizeof(void*));
  if (DestroyValueStruct != NULL) {
    (*DestroyValueStruct)(&currentValue);
  }
  free(bucketData);
}

// Creates a new bucket and makes the given one point to the new one
int Bucket_CreateNext(char *lastBucketData) {
  // Create new bucket
  char *newBucketData;
  if ((newBucketData = CreateBucket()) == NULL) {
    return FALSE;
  }
  // Copy pointer for the new bucket to the one given
  memcpy(lastBucketData + BUCKET_SIZE - sizeof(char*),&newBucketData,sizeof(char*));
  return TRUE;
}

char* Bucket_Next(char *bucketData) {
  char *ret;
  memcpy(&ret,bucketData + BUCKET_SIZE - sizeof(char*),sizeof(char*));
  return ret;
}

void Bucket_ChangeNext(char *bucketData,char *newNext) {
  memcpy(bucketData + BUCKET_SIZE - sizeof(char*),newNext,sizeof(void*));
}

int HashTable_Insert(HashTable table,string key,void *value) {
  if (value == NULL) {
    return FALSE;
  }
  unsigned int entry = hash(key,table->numOfEntries);
  // First record for this entry
  if (table->entries[entry] == NULL) {
    char *newBucket;
    if ((newBucket = CreateBucket()) == NULL) {
      not_enough_memory();
      return FALSE;
    }
    Bucket_InsertRecord(newBucket,key,value);
    table->entries[entry] = newBucket;
    return TRUE;
  } else {
    // Not 1st record so iterate through the buckets until we find an available place
    char *currentBucket = table->entries[entry];
    char *previousBucket = NULL;
    while (currentBucket != NULL) {
      if (Bucket_InsertRecord(currentBucket,key,value)) {
        // Successful insertion in the current bucket
        return TRUE;
      } else {
        // Otherwise move to the next bucket keeping pointer to the previous one
        previousBucket = currentBucket;
        currentBucket = Bucket_Next(currentBucket);
      }
    }
    // No available space in current buckets so create a new one and insert the record there
    if (Bucket_CreateNext(previousBucket)) {
      currentBucket = Bucket_Next(previousBucket);
      return Bucket_InsertRecord(currentBucket,key,value);
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
      if ((currentValue = Bucket_SearchKey(currentBucket,key)) != NULL) {
        // Successful insertion in the current bucket
        return currentValue;
      } else {
        // Otherwise move to the next bucket
        currentBucket = Bucket_Next(currentBucket);
      }
    }
    // Not found
    return NULL;
  } else {
    // Not found(no bucket in current entry)
    return NULL;
  }
}

int HashTable_Delete(HashTable table,string key,int (*DestroyValueStruct)(void**)) {
  unsigned int entry = hash(key,table->numOfEntries);
  if (table->entries[entry] != NULL) {
    char *currentBucket = table->entries[entry];
    char *previousBucket = NULL;
    while (currentBucket != NULL) {
      void *currentValue;
      if ((currentValue = Bucket_SearchKey(currentBucket,key)) != NULL) {
        // Key found in current bucket
        // Make the previous bucket point to the next one (or the entries table if current bucket is the first one in the list)
        if (previousBucket != NULL) {
          Bucket_ChangeNext(previousBucket,Bucket_Next(currentBucket));
        } else {
          table->entries[entry] = Bucket_Next(currentBucket);
        }
        // Destroy the current bucket
        Bucket_Destroy(currentBucket,DestroyValueStruct);
        return TRUE;
      } else {
        // Otherwise move to the next bucket
        previousBucket = currentBucket;
        currentBucket = Bucket_Next(currentBucket);
      }
    }
    // Not found
    return FALSE;
  } else {
    // Not found(no bucket in current entry)
    return FALSE;
  }
}

int HashTable_ReplaceKeyValue(HashTable table,string key,int (*DestroyValueStruct)(void**),void *newValue) {
  unsigned int entry = hash(key,table->numOfEntries);
  if (table->entries[entry] != NULL) {
    char *currentBucket = table->entries[entry];
    while (currentBucket != NULL) {
      if (Bucket_ReplaceKey(currentBucket,key,DestroyValueStruct,newValue)) {
        // Successful insertion in the current bucket
        return TRUE;
      } else {
        // Otherwise move to the next bucket
        currentBucket = Bucket_Next(currentBucket);
      }
    }
    // Not found
    return FALSE;
  } else {
    // Not found(no bucket in current entry)
    return FALSE;
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
        char *nextBucket = Bucket_Next(currentBucket);
        va_list valist;
        va_start(valist,argc);
        // Get key
        string currentKey;
        memcpy(&currentKey,currentBucket,sizeof(string));
        if (currentKey != NULL) {
          // Get value
          void *currentValue;
          memcpy(&currentValue,currentBucket + sizeof(string),sizeof(void*));
          (*func)(currentKey,currentValue,argc,valist);
          va_end(valist);
        } else {
          va_end(valist);
          break;
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
        char *nextBucket = Bucket_Next(currentBucket);
        Bucket_Destroy(currentBucket,DestroyValueStruct);
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
