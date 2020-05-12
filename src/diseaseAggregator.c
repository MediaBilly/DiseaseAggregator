#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include "../headers/hashtable.h"
#include "../headers/utils.h"
#include "../headers/list.h"

#define FREE_DA_MEMORY() free(input_dir);

void usage() {
  fprintf(stderr,"Usage:./diseaseAggregator –w numWorkers -b bufferSize -i input_dir\n");
}

int main(int argc, char const *argv[]) {
  // Define arguments and attributes
  unsigned int numWorkers,bufferSize;
  string input_dir;
  // Create hashtable that maps countries to PID's
  HashTable countryToPidMap;
  if (!HashTable_Create(&countryToPidMap,200,100)) {
    return 1;
  }
  // Check usage
  if (argc != 7) {
    usage();
    return 1;
  }
  // Read num workers
  if (!strcmp(argv[1],"-w")) {
    numWorkers = atoi(argv[2]);
  } else {
    usage();
    return 1;
  }
  // Read buffer size
  if (!strcmp(argv[3],"-b")) {
    bufferSize = atoi(argv[4]);
  } else {
    usage();
    return 1;
  }
  // Read input_dir
  if (!strcmp(argv[5],"-i")) {
    if ((input_dir = (string)malloc(strlen(argv[6] + 1))) != NULL) {
      strcpy(input_dir,argv[6]);
    } else {
      not_enough_memory();
      return 1;
    }
  }
  // Check if numWorkers is > 0
  if (numWorkers == 0) {
    fprintf(stderr,"numWorkers must be > 0\n");
    FREE_DA_MEMORY()
    return 1;
  }
  // Check if bufferSize is > 0
  if (bufferSize == 0) {
    fprintf(stderr,"bufferSize must be > 0\n");
    FREE_DA_MEMORY()
    return 1;
  }
  // Check if input_dir is really a directory
  struct stat input_dir_info;
  if (stat(input_dir,&input_dir_info) != -1) {
    if ((input_dir_info.st_mode & S_IFMT) != S_IFDIR) {
      fprintf(stderr,"%s is not a directory\n",input_dir);
      FREE_DA_MEMORY()
      return 1;
    }
  } else {
    perror("Failed to get input_dir info");
    FREE_DA_MEMORY()
    return 1;
  }
  // Open input_dir
  DIR *input_dir_ptr;
  struct dirent *direntp;
  unsigned int totalCountries = 0;
  // Create countries list
  List countryList;
  if ((!List_Initialize(&countryList))) {
    FREE_DA_MEMORY()
    return 1;
  }
  if ((input_dir_ptr = opendir(input_dir)) != NULL) {
    // Scan and count all country directories in input_dir
    while ((direntp = readdir(input_dir_ptr)) != NULL) {
      if (strcmp(direntp->d_name,".") && strcmp(direntp->d_name,"..") && direntp->d_type == DT_DIR) {
        // Add country to the list
        List_Insert(countryList,direntp->d_name);
        //printf("%s\n",direntp->d_name);
        totalCountries++;
      }
    }
    closedir(input_dir_ptr);
  } else {
    fprintf(stderr,"cannot open %s\n",input_dir);
    FREE_DA_MEMORY()
    return 1;
  }
  //pid_t workers[numWorkers];
  char write_fifo[numWorkers][20];
  char read_fifo[numWorkers][20];
  unsigned int i,j,workerCountries,currentCountries = totalCountries;
  pid_t pid;
  // Create an iterator for the countries list
  ListIterator countriesIt = List_CreateIterator(countryList);
  // Create worker processes and distribute country directories to them
  for (i = 0;i < numWorkers;i++) {
    // Create named pipes for the current worker process
    sprintf(read_fifo[i],"worker_r%d",i);
    if (mkfifo(read_fifo[i],FIFO_PERMS) < 0) {
      perror("Fifo creation error");
      FREE_DA_MEMORY()
      return 1;
    }
    sprintf(write_fifo[i],"worker_w%d",i);
    if (mkfifo(write_fifo[i],FIFO_PERMS) < 0) {
      perror("Fifo creation error");
      FREE_DA_MEMORY()
      return 1;
    }
    if ((pid = fork()) == -1) {
      perror("Fork failed");
      FREE_DA_MEMORY()
      return 1;
    }
    // Child
    else if (pid == 0) {
      execl("./worker","worker",read_fifo[i],write_fifo[i],NULL);
      perror("Exec failed");
      return 1;
    }
    // Parent
    else {
      // Save current worker's pid for later use
      //workers[i] = pid;
      // Calculate # of countries for the current worker using uniforn distribution round robbin algorithm
      workerCountries = CEIL(currentCountries,numWorkers - i);
      // Distribute country directories to the worker process
      for (j = 0;j < workerCountries && countriesIt != NULL;j++) {
        // Insert country to the countries hashTable
        if (!HashTable_Insert(countryToPidMap,ListIterator_GetValue(countriesIt),&pid)) {
          FREE_DA_MEMORY()
          return 1;
        }
        printf("Worker %d took %s\n",pid,ListIterator_GetValue(countriesIt));
        totalCountries++;
        ListIterator_MoveToNext(&countriesIt);
      }
      printf("Worker %u took %u countries\n",i+1,workerCountries);
      currentCountries -= workerCountries; 
    }
  }
  // Wait for workers to finish execution
  for (i = 0;i < numWorkers;i++) {
    int exit_status;
    pid_t exited_pid;
    if ((exited_pid = wait(&exit_status)) == -1) {
      perror("Wait failed");
      return 1;
    }
  }
  // Destroy all fifo's
  for (i = 0;i < numWorkers;i++) {
    unlink(read_fifo[i]);
    unlink(write_fifo[i]);
  }
  printf("%u %u %s %u\n",numWorkers,bufferSize,input_dir,totalCountries);
  FREE_DA_MEMORY()
  return 0;
}
