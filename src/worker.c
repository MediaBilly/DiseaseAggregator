#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "../headers/utils.h"

int main(int argc, char const *argv[]) {
  if (argc != 5) {
    fprintf(stderr,"Usage:./worker fifo_aggregator_to_worker fifo_worker_to_aggregator input_dir bufferSize\n");
    return 1;
  }
  //printf("%s %s\n",argv[1],argv[2]);
  // Read arguments
  string fifo_worker_to_aggregator = (string) argv[2];//,fifo_aggregator_to_worker = argv[1],input_dir = argv[3]; 
  unsigned int bufferSize = atoi(argv[4]);
  // Open fifo_aggregator_to_worker to read the country directory names to be opened
  int fifo_worker_to_aggregator_fd = open(fifo_worker_to_aggregator,O_RDONLY);
  // Read the countries sent by diseaseAggregator
  char *countries = receive_data(fifo_worker_to_aggregator_fd,bufferSize);
  if (countries != NULL) {
    string country = strtok(countries,"\n");
    while (country != NULL) {
      printf("%d %s\n",getpid(),country);
      country = strtok(NULL,"\n");
    }
    //printf("%s\n",countries);
  }
  free(countries);
  close(fifo_worker_to_aggregator_fd);
  return 0;
}
