#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include "../headers/utils.h"

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
    if ((input_dir_info.st_mode & S_IFMT) != S_IFDIR ) {
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
  if ((input_dir_ptr = opendir(input_dir)) != NULL) {
    // Scan and count all country directories in input_dir
    while ((direntp = readdir(input_dir_ptr)) != NULL) {
      if (strcmp(direntp->d_name,".") && strcmp(direntp->d_name,"..") && direntp->d_type == DT_DIR) {
        //printf("%s\n",direntp->d_name);
        totalCountries++;
      }
    }
    closedir(input_dir_ptr);
  } else {
    fprintf(stderr,"cannot open %s\n",input_dir);
    free(input_dir);
    return 1;
  }
  // Distribute country directories to all the workers
  unsigned int i,workerCountries,currentCountries = totalCountries;
  for (i = 0;i < numWorkers;i++) {
    workerCountries = CEIL(currentCountries,numWorkers - i);
    printf("Worker %u takes %u countries\n",i+1,workerCountries);
    currentCountries -= workerCountries;
  }
  printf("%u %u %s %u\n",numWorkers,bufferSize,input_dir,totalCountries);
  free(input_dir);
  return 0;
}
