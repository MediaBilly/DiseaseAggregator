#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include "../headers/hashtable.h"
#include "../headers/utils.h"
#include "../headers/list.h"

// Maps countries to pids
HashTable countryToPidMap;
// Maps a pid to a list of all it's countries
HashTable pidCountriesHT;
// Maps a pid to it's fifo_aggregator_to_worker
HashTable pid_fifo_aggregator_to_workerHT;
// Maps a pid to it's fifo_worker_to_aggregator
HashTable pid_fifo_worker_to_aggregatorHT;
List countryList;
boolean running;
string input_dir;
unsigned int TOTAL,SUCCESS;
unsigned int numWorkers,bufferSize;
// Temporary set of pipe descriptors used by select() function
fd_set pipe_fd_set;
int maxFd;

void usage() {
  fprintf(stderr,"Usage:./diseaseAggregator –w numWorkers -b bufferSize -i input_dir\n");
}

void printCountryPIDs(string country,void *pidptr,int argc,va_list valist) {
  pid_t pid = *(pid_t*)pidptr;
  printf("%s %d\n",country,pid);
}

void destroyCountriesHT(string country,void *pidptr,int argc,va_list valist) {
  free(pidptr);
}

void finish_execution(int signum) {
  signal(SIGINT,finish_execution);
  signal(SIGQUIT,finish_execution);
  running = FALSE;
  fclose(stdin);
}

void respawn_worker() {
  signal(SIGCHLD,respawn_worker);
  // Get the pid of the exited process
  pid_t exitedPID = wait(NULL);
  char pidStr[sizeof(pid_t) + 1];
  sprintf(pidStr,"%d",exitedPID);
  // Get the countries list of the exited pid
  List countriesList = HashTable_SearchKey(pidCountriesHT,pidStr);
  // Delete old entry from pidCountriesHT
  HashTable_Delete(pidCountriesHT,pidStr,NULL);
  // Get fifo's from hash tables to give the same to the new child
  string fifo_aggregator_to_worker = HashTable_SearchKey(pid_fifo_aggregator_to_workerHT,pidStr);
  string fifo_worker_to_aggregator = HashTable_SearchKey(pid_fifo_worker_to_aggregatorHT,pidStr);
  // Delete them from the hash table (because they are mapped with the old pid)
  HashTable_Delete(pid_fifo_aggregator_to_workerHT,pidStr,NULL);
  HashTable_Delete(pid_fifo_worker_to_aggregatorHT,pidStr,NULL);
  // Fork new worker process
  pid_t newPID = fork();
  // Fork error
  if (newPID == -1) {
    perror("Fork Failed");
  }
  // Child
  else if (newPID == 0) {
    char bufSize[10];
    sprintf(bufSize,"%d",bufferSize);
    execl("./worker","worker",fifo_aggregator_to_worker,fifo_worker_to_aggregator,input_dir,bufSize,"-nostats",NULL);
    perror("Exec failed");
    exit(1);
  }
  // Parent
  else {
    // Create an entry for the new pid's country list and insert the old list as value
    char newPidStr[sizeof(pid_t) + 1];
    sprintf(newPidStr,"%d",newPID);
    if (!HashTable_Insert(pidCountriesHT,newPidStr,countriesList)) {
      kill(newPID,SIGKILL);
    }
    // Send the old worker's countries to the new one
    ListIterator it = List_CreateIterator(countriesList);
    int fd = open(fifo_aggregator_to_worker,O_WRONLY);
    while (it != NULL) {
      pid_t *pidPtr;
      if ((pidPtr = (pid_t*)malloc(sizeof(pid_t))) == NULL) {
        close(fd);
        kill(newPID,SIGKILL);
      }
      *pidPtr = newPID;
      char country[strlen(ListIterator_GetValue(it)) + 1];
      memcpy(country,ListIterator_GetValue(it),strlen(ListIterator_GetValue(it)) + 1);
      country[strlen(ListIterator_GetValue(it))] = '\n';
      send_data(fd,country,sizeof(country),bufferSize);
      free(HashTable_SearchKey(countryToPidMap,ListIterator_GetValue(it)));
      HashTable_ReplaceKeyValue(countryToPidMap,ListIterator_GetValue(it),NULL,pidPtr);
      ListIterator_MoveToNext(&it);
    }
    close(fd);
    // Reinsert fifos to the hash tables with the new pid this time
    if (!HashTable_Insert(pid_fifo_aggregator_to_workerHT,newPidStr,fifo_aggregator_to_worker) || !HashTable_Insert(pid_fifo_worker_to_aggregatorHT,newPidStr,fifo_worker_to_aggregator)) {
      close(fd);
      kill(newPID,SIGKILL);
    }
  }
}

void kill_all_workers(string pidstr,void *listptr,int argc,va_list valist) {
  pid_t pid = atoi(pidstr);
  //printf("KILLING: %d\n",pid);
  kill(pid,SIGINT);// THE ASSIGNMENT WANTS SIGKILL FOR SOME REASON BUT IT'S AN IRREGULAR METHOD SO SEND SIGINT FOR THE MOMENT
}

void middle_free() {
  HashTable_Destroy(&pid_fifo_worker_to_aggregatorHT,NULL);
  HashTable_Destroy(&pid_fifo_aggregator_to_workerHT,NULL);
  HashTable_Destroy(&pidCountriesHT,NULL);
  HashTable_Destroy(&countryToPidMap,NULL);
  List_Destroy(&countryList);
  free(input_dir);
}

void send_data_to_all_workers(string pidStr,void *fifo,int argc,va_list valist) {
  char* data = va_arg(valist,char*);
  unsigned int dataSize = va_arg(valist,unsigned int);
  int fd = open((const char*)fifo,O_WRONLY);
  send_data(fd,data,dataSize,bufferSize);
  close(fd);
}

// Reads and combines results from all workers for the diseaseFrequency query
void diseaseFrequencyAllCountries(string pidStr,void *fifo,int argc,va_list valist) {
  unsigned int *result = va_arg(valist,unsigned int*);
  int fd = open((const char*)fifo,O_RDONLY);
  char *receivedData = receive_data(fd,bufferSize,TRUE);
  *result += atoi(receivedData);
  free(receivedData);
  close(fd);
}

void searchPatientRecord(string pidStr,void *fifo,int argc,va_list valist) {
  boolean *found = va_arg(valist,boolean*);
  int fd = open((const char*)fifo,O_RDONLY);
  char *receivedData = receive_data(fd,bufferSize,TRUE);
  if (strcmp(receivedData,"nf")) {
    printf("%s",receivedData);
    *found = TRUE;
  }
  free(receivedData);
  close(fd);
}

void printResultsFromAllWorkers(string pidStr,void *fifo,int argc,va_list valist) {
  int fd = open((const char*)fifo,O_RDONLY);
  char *receivedData = receive_data(fd,bufferSize,TRUE);
  printf("%s",receivedData);
  free(receivedData);
  close(fd);
}

int main(int argc, char const *argv[]) {
  // Global initializations
  FD_ZERO(&pipe_fd_set);
  // Register signal handlers
  signal(SIGINT,finish_execution);
  signal(SIGQUIT,finish_execution);
  signal(SIGCHLD,respawn_worker);
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
  // Check if 0 < buffer size is <= pipe size
  if (0 > bufferSize || bufferSize > pipe_size()) {
    printf("Invalid buffer size.\n");
    free(input_dir);
    return 1;
  }
  // Open input_dir
  DIR *input_dir_ptr;
  struct dirent *direntp;
  unsigned int totalCountries = 0;
  // Create countries list
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
  char fifo_aggregator_to_worker[numWorkers][20];
  char fifo_worker_to_aggregator[numWorkers][20];
  unsigned int i,j,workerCountries,currentCountries = totalCountries;
  pid_t pid;
  // Create an iterator for the countries list
  ListIterator countriesIt = List_CreateIterator(countryList);
  // Create the hashtable that maps countries to PID's
  if (!HashTable_Create(&countryToPidMap,200)) {
    List_Destroy(&countryList);
    free(input_dir);
    return 1;
  }
  // Create the hashtable that maps a pid to a list of all it's countries
  if (!HashTable_Create(&pidCountriesHT,200)) {
    HashTable_Destroy(&countryToPidMap,NULL);
    List_Destroy(&countryList);
    free(input_dir);
    return 1;
  }
  // Create the hashtable that maps a pid to it's fifo_aggregator_to_worker
  if (!HashTable_Create(&pid_fifo_aggregator_to_workerHT,200)) {
    HashTable_Destroy(&pidCountriesHT,NULL);
    HashTable_Destroy(&countryToPidMap,NULL);
    List_Destroy(&countryList);
    free(input_dir);
    return 1;
  }
  // Create the hashtable that maps a pid to it's fifo_worker_to_aggregator
  if (!HashTable_Create(&pid_fifo_worker_to_aggregatorHT,200)) {
    HashTable_Destroy(&pid_fifo_worker_to_aggregatorHT,NULL);
    HashTable_Destroy(&pidCountriesHT,NULL);
    HashTable_Destroy(&countryToPidMap,NULL);
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
      middle_free();
      return 1;
    }
    sprintf(fifo_aggregator_to_worker[i],"worker_r%d",i);
    if (mkfifo(fifo_aggregator_to_worker[i],FIFO_PERMS) < 0) {
      perror("Fifo creation error");
      middle_free();
      return 1;
    }
    if ((pid = fork()) == -1) {
      perror("Fork failed");
      middle_free();
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
      // Calculate # of countries for the current worker using uniforn distribution round robbin algorithm
      workerCountries = CEIL(currentCountries,numWorkers - i);
      // Open pipe for writing to the worker process
      int fd = open(fifo_aggregator_to_worker[i],O_WRONLY);
      // Create a list of the countries that this process will work on
      List pidCountries;
      if (!List_Initialize(&pidCountries)) {
        middle_free();
        close(fd);
        return 1;
      }
      // Insert pid and the list above to the pidCountries hash table
      char pidStr[sizeof(pid_t) + 1];
      sprintf(pidStr,"%d",pid);
      if (!HashTable_Insert(pidCountriesHT,pidStr,pidCountries)) {
        List_Destroy(&pidCountries);
        middle_free();
        close(fd);
        return 1;
      }
      // Insert the fifo's to the hashtables
      string fifo1Copy = CopyString(fifo_aggregator_to_worker[i]);
      if (!HashTable_Insert(pid_fifo_aggregator_to_workerHT,pidStr,fifo1Copy)) {
        DestroyString(&fifo1Copy);
        List_Destroy(&pidCountries);
        middle_free();
        close(fd);
        return 1;
      }
      string fifo2Copy = CopyString(fifo_worker_to_aggregator[i]);
      if (!HashTable_Insert(pid_fifo_worker_to_aggregatorHT,pidStr,fifo2Copy)) {
        DestroyString(&fifo2Copy);
        DestroyString(&fifo1Copy);
        List_Destroy(&pidCountries);
        middle_free();
        close(fd);
        return 1;
      }
      // Distribute country directories to the worker process
      for (j = 0;j < workerCountries && countriesIt != NULL;j++) {
        // Insert country to the countries hashTable
        pid_t *pidPtr;
        if ((pidPtr = (pid_t*)malloc(sizeof(pid_t))) == NULL) {
          DestroyString(&fifo2Copy);
          DestroyString(&fifo1Copy);
          not_enough_memory();
          middle_free();
          close(fd);
          return 1;
        }
        memcpy(pidPtr,&pid,sizeof(pid_t));
        if (!HashTable_Insert(countryToPidMap,ListIterator_GetValue(countriesIt),pidPtr)) {
          DestroyString(&fifo2Copy);
          DestroyString(&fifo1Copy);
          middle_free();
          close(fd);
          return 1;
        }
        // Insert country to the hash table's list
        if (!List_Insert(pidCountries,ListIterator_GetValue(countriesIt))) {
          DestroyString(&fifo2Copy);
          DestroyString(&fifo1Copy);
          middle_free();
          close(fd);
          return 1;
        }
        // Send country to worker process
        char country[strlen(ListIterator_GetValue(countriesIt)) + 1];
        memcpy(country,ListIterator_GetValue(countriesIt),strlen(ListIterator_GetValue(countriesIt)) + 1);
        country[strlen(ListIterator_GetValue(countriesIt))] = '\n';
        send_data(fd,country,sizeof(country),bufferSize);
        totalCountries++;
        ListIterator_MoveToNext(&countriesIt);
      }
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
  running = TRUE;
  string line = NULL,command;
  string *args;
  size_t len;
  while (running) {
    putchar('>');
    // Read command name
    if(getline(&line,&len,stdin) == -1) {
      if (line != NULL) {
        free(line);
      }
      running = FALSE;
      break;
    }
    IgnoreNewLine(line);
    if (strlen(line) == 0) {
      DestroyString(&line);
      continue;
    }
    string fullCommand = CopyString(line);
    argc = wordCount(line);
    args = SplitString(line," ");
    command = args[0];
    // Determine command type
    if (!strcmp("/listCountries",command)) {
      TOTAL += 1;
      // Usage check 
      if (argc == 1) {
        SUCCESS += 1;
        HashTable_ExecuteFunctionForAllKeys(countryToPidMap,printCountryPIDs,0);
      } else {
        printf("Usage:/listCountries\n");
      }
    } else if (!strcmp("/exit",command)) {
      // Usage check
      if (argc == 1) {
        running = FALSE;
      } else {
        printf("Usage:/exit\n");
      }
    } else if (!strcmp("/diseaseFrequency",command)) {
      TOTAL += 1;
      // Usage check
      if (argc == 5) {
        // Country given so query the worker which is responsible for that country
        // Get the country
        string country = args[4];
        // Get responsible pid
        pid_t *queryPID = HashTable_SearchKey(countryToPidMap,country);
        if (queryPID != NULL) {
          char queryPidStr[sizeof(pid_t) + 1];
          sprintf(queryPidStr,"%d",*queryPID);
          // Send the command to the worker
          int fd = open(HashTable_SearchKey(pid_fifo_aggregator_to_workerHT,queryPidStr),O_WRONLY);
          send_data(fd,fullCommand,strlen(fullCommand),bufferSize);
          close(fd);
          // Get the answer
          int readFd = open(HashTable_SearchKey(pid_fifo_worker_to_aggregatorHT,queryPidStr),O_RDONLY);
          char *answer = receive_data(readFd,bufferSize,TRUE);
          printf("%s\n",answer);
          free(answer);
          close(readFd);
          SUCCESS += 1;
        } else {
          fprintf(stderr,"Country %s not found.\n",country);
        }
      } else if (argc == 4) {
        // No country given so request all the workers
        // Send the command to all the workers
        HashTable_ExecuteFunctionForAllKeys(pid_fifo_aggregator_to_workerHT,send_data_to_all_workers,2,fullCommand,strlen(fullCommand));
        // Read results from all workers, sum them and print the final sum
        unsigned int result = 0;
        HashTable_ExecuteFunctionForAllKeys(pid_fifo_worker_to_aggregatorHT,diseaseFrequencyAllCountries,1,&result);
        printf("%u\n",result);
        SUCCESS += 1;
      } else {
        printf("Usage:/diseaseFrequency virusName date1 date2 [country]\n");
      }
    } else if (!strcmp("/topk-AgeRanges",command)) {
      TOTAL += 1;
      // Usage check
      if (argc == 6) {
        // Get the country
        string country = args[2];
        // Get responsible pid
        pid_t *queryPID = HashTable_SearchKey(countryToPidMap,country);
        if (queryPID != NULL) {
          char queryPidStr[sizeof(pid_t) + 1];
          sprintf(queryPidStr,"%d",*queryPID);
          // Send the command to the worker
          int fd = open(HashTable_SearchKey(pid_fifo_aggregator_to_workerHT,queryPidStr),O_WRONLY);
          send_data(fd,fullCommand,strlen(fullCommand),bufferSize);
          close(fd);
          // Get the answer
          int readFd = open(HashTable_SearchKey(pid_fifo_worker_to_aggregatorHT,queryPidStr),O_RDONLY);
          char *answer = receive_data(readFd,bufferSize,TRUE);
          printf("%s",answer);
          free(answer);
          close(readFd);
          SUCCESS += 1;
        } else {
          fprintf(stderr,"Country %s not found.\n",country);
        }
      } else {
        printf("Usage:/topk-AgeRanges k country disease date1 date2\n");
      }
    } else if (!strcmp("/searchPatientRecord",command)) {
      TOTAL += 1;
      // Usage check
      if (argc == 2) {
        // Send the command to all the workers
        HashTable_ExecuteFunctionForAllKeys(pid_fifo_aggregator_to_workerHT,send_data_to_all_workers,2,fullCommand,strlen(fullCommand));
        boolean found = FALSE;
        // Wait for answers
        HashTable_ExecuteFunctionForAllKeys(pid_fifo_worker_to_aggregatorHT,searchPatientRecord,1,&found);
        // If not found print NOT FOUND
        if (!found) {
          printf("NOT FOUND\n");
        }
        SUCCESS += 1;
      } else {
        printf("Usage:/searchPatientRecord recordID \n");
      }
    } else if (!strcmp("/numPatientAdmissions",command)) {
      TOTAL += 1;
      // Usage check
      if (argc == 5) {
        // Country given so query the worker which is responsible for that country
        // Get the country
        string country = args[4];
        // Get responsible pid
        pid_t *queryPID = HashTable_SearchKey(countryToPidMap,country);
        if (queryPID != NULL) {
          char queryPidStr[sizeof(pid_t) + 1];
          sprintf(queryPidStr,"%d",*queryPID);
          // Send the command to the worker
          int fd = open(HashTable_SearchKey(pid_fifo_aggregator_to_workerHT,queryPidStr),O_WRONLY);
          send_data(fd,fullCommand,strlen(fullCommand),bufferSize);
          close(fd);
          // Get the answer
          int readFd = open(HashTable_SearchKey(pid_fifo_worker_to_aggregatorHT,queryPidStr),O_RDONLY);
          char *answer = receive_data(readFd,bufferSize,TRUE);
          printf("%s",answer);
          free(answer);
          close(readFd);
          SUCCESS += 1;
        } else {
          fprintf(stderr,"Country %s not found.\n",country);
        }
      } else if (argc == 4) {
        // No country given so request all the workers
        // Send the command to all the workers
        HashTable_ExecuteFunctionForAllKeys(pid_fifo_aggregator_to_workerHT,send_data_to_all_workers,2,fullCommand,strlen(fullCommand));
        // Read results from all workers, sum them and print the final sum
        HashTable_ExecuteFunctionForAllKeys(pid_fifo_worker_to_aggregatorHT,printResultsFromAllWorkers,0);
        SUCCESS += 1;
      } else {
        printf("Usage:/numPatientAdmissions disease date1 date2 [country]\n");
      }
    } else if (!strcmp("/numPatientDischarges",command)) {
      TOTAL += 1;
      // Usage check
      if (argc == 5) {
        // Country given so query the worker which is responsible for that country
        // Get the country
        string country = args[4];
        // Get responsible pid
        pid_t *queryPID = HashTable_SearchKey(countryToPidMap,country);
        if (queryPID != NULL) {
          char queryPidStr[sizeof(pid_t) + 1];
          sprintf(queryPidStr,"%d",*queryPID);
          // Send the command to the worker
          int fd = open(HashTable_SearchKey(pid_fifo_aggregator_to_workerHT,queryPidStr),O_WRONLY);
          send_data(fd,fullCommand,strlen(fullCommand),bufferSize);
          close(fd);
          // Get the answer
          int readFd = open(HashTable_SearchKey(pid_fifo_worker_to_aggregatorHT,queryPidStr),O_RDONLY);
          char *answer = receive_data(readFd,bufferSize,TRUE);
          printf("%s",answer);
          free(answer);
          close(readFd);
          SUCCESS += 1;
        } else {
          fprintf(stderr,"Country %s not found.\n",country);
        }
      } else if (argc == 4) {
        // No country given so request all the workers
        // Send the command to all the workers
        HashTable_ExecuteFunctionForAllKeys(pid_fifo_aggregator_to_workerHT,send_data_to_all_workers,2,fullCommand,strlen(fullCommand));
        // Read results from all workers, sum them and print the final sum
        HashTable_ExecuteFunctionForAllKeys(pid_fifo_worker_to_aggregatorHT,printResultsFromAllWorkers,0);
        SUCCESS += 1;
      } else {
        printf("Usage:/numPatientDischarges disease date1 date2 [country]\n");
      }
    } else {
      printf("Command %s not found.\n",command);
    }
    // Free some memory
    free(args);
    DestroyString(&line);
    DestroyString(&fullCommand);
  }
  // Unregister SIGCHLD handler because when the aggregator finishes execution we do not need to respawn children
  signal(SIGCHLD,SIG_DFL);
  // Kill all the processes and close all the fifo's
  HashTable_ExecuteFunctionForAllKeys(pidCountriesHT,kill_all_workers,0);
  // Wait for workers to finish execution
  for (i = 0;i < numWorkers;i++) {
    int exit_status;
    pid_t exited_pid;
    if ((exited_pid = wait(&exit_status)) == -1) {
      perror("Wait failed");
    }
  }
  // Destroy all fifo's
  for (i = 0;i < numWorkers;i++) {
    unlink(fifo_worker_to_aggregator[i]);
    unlink(fifo_aggregator_to_worker[i]);
  }
  // Create logfile
  char filename[10 + sizeof(pid_t)];
  sprintf(filename,"log_file.%d",getpid());
  FILE *output = fopen(filename,"w");
  countriesIt = List_CreateIterator(countryList);
  while (countriesIt != NULL) {
    fprintf(output,"%s\n",ListIterator_GetValue(countriesIt));
    ListIterator_MoveToNext(&countriesIt);
  }
  fprintf(output,"TOTAL %u\nSUCCESS %u\nFAIL %u\n",TOTAL,SUCCESS,TOTAL - SUCCESS);
  fclose(output);
  HashTable_ExecuteFunctionForAllKeys(countryToPidMap,destroyCountriesHT,0);
  HashTable_Destroy(&pid_fifo_worker_to_aggregatorHT,(int (*)(void**))DestroyString);
  HashTable_Destroy(&pid_fifo_aggregator_to_workerHT,(int (*)(void**))DestroyString);
  HashTable_Destroy(&pidCountriesHT,(int (*)(void**))List_Destroy);
  HashTable_Destroy(&countryToPidMap,NULL);
  List_Destroy(&countryList);
  free(input_dir);
  return 0;
}
