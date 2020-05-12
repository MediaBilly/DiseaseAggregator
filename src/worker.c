#include <stdio.h>

int main(int argc, char const *argv[]) {
  if (argc != 4) {
    fprintf(stderr,"Usage:./worker readFIFO writeFIFO input_dir\n");
    return 1;
  }
  printf("%s %s\n",argv[1],argv[2]);
  return 0;
}
