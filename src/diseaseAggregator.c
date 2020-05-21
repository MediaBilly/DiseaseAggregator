#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include "../headers/hashtable.h"
#include "../headers/utils.h"
#include "../headers/list.h"

void usage() {
  fprintf(stderr,"Usage:./diseaseAggregator –w numWorkers -b bufferSize -i input_dir\n");
}

int main(int argc, char const *argv[]) {
  // Define arguments and attributes
  unsigned int numWorkers,bufferSize;
  string input_dir;
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
    if ((input_dir = CopyString((string)argv[6])) == NULL) {
      not_enough_memory();
      return 1;
    }
  }
  // Check if numWorkers is > 0
  if (numWorkers == 0) {
    fprintf(stderr,"numWorkers must be > 0\n");
    free(input_dir);
    return 1;
  }
  // Check if bufferSize is > 0
  if (bufferSize == 0) {
    fprintf(stderr,"bufferSize must be > 0\n");
    free(input_dir);
    return 1;
  }
  // Check if input_dir is really a directory
  struct stat input_dir_info;
  if (stat(input_dir,&input_dir_info) != -1) {
    if ((input_dir_info.st_mode & S_IFMT) != S_IFDIR) {
      fprintf(stderr,"%s is not a directory\n",input_dir);
      free(input_dir);
      return 1;
    }
  } else {
    perror("Failed to get input_dir info");
    free(input_dir);
    return 1;
  }
  // Open input_dir
  DIR *input_dir_ptr;
  struct dirent *direntp;
  unsigned int totalCountries = 0;
  // Create countries list
  List countryList;
  if ((!List_Initialize(&countryList))) {
    free(input_dir);
    return 1;
  }
  if ((input_dir_ptr = opendir(input_dir)) != NULL) {
    // Scan and count all country directories in input_dir
    while ((direntp = readdir(input_dir_ptr)) != NULL) {
      if (strcmp(direntp->d_name,".") && strcmp(direntp->d_name,"..") && direntp->d_type == DT_DIR) {
        // Add country to the list
        if (!List_Insert(countryList,direntp->d_name)) {
          List_Destroy(&countryList);
          free(input_dir);
          return 1;
        }
        //printf("%s\n",direntp->d_name);
        totalCountries++;
      }
    }
    closedir(input_dir_ptr);
  } else {
    fprintf(stderr,"cannot open %s\n",input_dir);
    List_Destroy(&countryList);
    free(input_dir);
    return 1;
  }
  //pid_t workers[numWorkers];
  char fifo_aggregator_to_worker[numWorkers][20];
  char fifo_worker_to_aggregator[numWorkers][20];
  unsigned int i,j,workerCountries,currentCountries = totalCountries;
  pid_t pid;
  // Create an iterator for the countries list
  ListIterator countriesIt = List_CreateIterator(countryList);
  // Create hashtable that maps countries to PID's
  HashTable countryToPidMap;
  if (!HashTable_Create(&countryToPidMap,200,100)) {
    List_Destroy(&countryList);
    free(input_dir);
    return 1;
  }
  // Create worker processes and distribute country directories to them
  for (i = 0;i < numWorkers;i++) {
    // Create named pipes for the current worker process
    sprintf(fifo_worker_to_aggregator[i],"worker_w%d",i);
    if (mkfifo(fifo_worker_to_aggregator[i],FIFO_PERMS) < 0) {
      perror("Fifo creation error");
      HashTable_Destroy(&countryToPidMap,NULL);
      List_Destroy(&countryList);
      free(input_dir);
      return 1;
    }
    sprintf(fifo_aggregator_to_worker[i],"worker_r%d",i);
    if (mkfifo(fifo_aggregator_to_worker[i],FIFO_PERMS) < 0) {
      perror("Fifo creation error");
      HashTable_Destroy(&countryToPidMap,NULL);
      List_Destroy(&countryList);
      free(input_dir);
      return 1;
    }
    if ((pid = fork()) == -1) {
      perror("Fork failed");
      HashTable_Destroy(&countryToPidMap,NULL);
      List_Destroy(&countryList);
      free(input_dir);
      return 1;
    }
    // Child
    else if (pid == 0) {
      char bufSize[10];
      sprintf(bufSize,"%d",bufferSize);
      execl("./worker","worker",fifo_aggregator_to_worker[i],fifo_worker_to_aggregator[i],input_dir,bufSize,NULL);
      perror("Exec failed");
      return 1;
    }
    // Parent
    else {
      // Save current worker's pid for later use
      //workers[i] = pid;
      // Calculate # of countries for the current worker using uniforn distribution round robbin algorithm
      workerCountries = CEIL(currentCountries,numWorkers - i);
      // Open pipe for writing to the worker process
      int fd = open(fifo_aggregator_to_worker[i],O_WRONLY);
      // Distribute country directories to the worker process
      for (j = 0;j < workerCountries && countriesIt != NULL;j++) {
        // Insert country to the countries hashTable
        if (!HashTable_Insert(countryToPidMap,ListIterator_GetValue(countriesIt),&pid)) {
          HashTable_Destroy(&countryToPidMap,NULL);
          List_Destroy(&countryList);
          free(input_dir);
          return 1;
        }
        // Send country to worker process
        char country[strlen(ListIterator_GetValue(countriesIt)) + 1];
        memcpy(country,ListIterator_GetValue(countriesIt),strlen(ListIterator_GetValue(countriesIt)) + 1);
        country[strlen(ListIterator_GetValue(countriesIt))] = '\n';
        send_data(fd,country,sizeof(country),bufferSize);
        //printf("Worker %d took %s\n",pid,ListIterator_GetValue(countriesIt));
        totalCountries++;
        ListIterator_MoveToNext(&countriesIt);
      }
      //printf("Worker %u took %u countries\n",i+1,workerCountries);
      currentCountries -= workerCountries;
      // Close the pipe
      close(fd);
    }
  }
  // Wait for statistics and ready state from all workers
  
  for (i = 0;i < numWorkers;i++) {
    int fd = open(fifo_worker_to_aggregator[i],O_RDONLY);
    char *statistics = receive_data(fd,bufferSize,TRUE);
    printf("%s\n",statistics);
    free(statistics);
    close(fd);
  }
  // Start command execution
  // Wait for workers to finish execution
  for (i = 0;i < numWorkers;i++) {
    int exit_status;
    pid_t exited_pid;
    if ((exited_pid = wait(&exit_status)) == -1) {
      perror("Wait failed");
      HashTable_Destroy(&countryToPidMap,NULL);
      List_Destroy(&countryList);
      free(input_dir);
      return 1;
    }
  }
  // Destroy all fifo's
  for (i = 0;i < numWorkers;i++) {
    unlink(fifo_worker_to_aggregator[i]);
    unlink(fifo_aggregator_to_worker[i]);
  }
  //printf("%u %u %s %u\n",numWorkers,bufferSize,input_dir,totalCountries);
  HashTable_Destroy(&countryToPidMap,NULL);
  List_Destroy(&countryList);
  free(input_dir);
  return 0;
}
