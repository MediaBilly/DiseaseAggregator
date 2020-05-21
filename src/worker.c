#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <signal.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include "../headers/list.h"
#include "../headers/hashtable.h"
#include "../headers/patientRecord.h"
#include "../headers/avltree.h"
#include "../headers/utils.h"

typedef struct {
  unsigned int years0to20;
  unsigned int years21to40;
  unsigned int years41to60;
  unsigned int years60plus;
} filestat;

List countriesList;
// Maps each country to another hash table that maps country's diseases an avl tree that contains their records sorted by age
HashTable recordsHT;
// Maps a record's ID to the record itself (used for /searchPatientRecord recordID command)
HashTable recordsByIdHT;

unsigned int bufferSize,TOTAL,SUCCESS;

void destroyCountryHTdiseaseTables(string disease,void *ht,int argc,va_list valist) {
  HashTable_Destroy((HashTable*)&ht,(int (*)(void**))AvlTree_Destroy);
}

void destroyDiseaseFilestatsFromHT(string filename,void *statsstruct,int argc,va_list valist) {
  free(statsstruct);
}

int digits(unsigned int num) {
  if (num == 0) {
    return 1;
  }
  int count = 0;
  while (num != 0) {
    count++;
    num/=10;
  }
  return count;
}

void summaryStatistics(string disease,void *statsstruct,int argc,va_list valist) {
  filestat *stats = (filestat*)statsstruct;
  int fd = va_arg(valist,int);
  char buf[strlen(disease) + 119 + digits(stats->years0to20) + digits(stats->years21to40) + digits(stats->years41to60) + digits(stats->years60plus)];
  memset(buf,0,sizeof(buf));
  sprintf(buf,"%s\nAge range 0-20 years: %d cases\nAge range 21-40 years: %d cases\nAge range 41-60 years: %d cases\nAge range 60+ years: %d cases\n\n",disease,stats->years0to20,stats->years21to40,stats->years41to60,stats->years60plus);
  send_data(fd,buf,sizeof(buf),strlen(buf));
}

void logfile(int signum) {
  signal(SIGINT,logfile);
  signal(SIGQUIT,logfile);
  char filename[10 + sizeof(pid_t)];
  sprintf(filename,"log_file.%d",getpid());
  FILE *output = fopen(filename,"w");
  ListIterator countriesIt = List_CreateIterator(countriesList);
  while (countriesIt != NULL) {
    fprintf(output,"%s\n",ListIterator_GetValue(countriesIt));
    ListIterator_MoveToNext(&countriesIt);
  }
  fprintf(output,"TOTAL %u\nSUCCESS %u\nFAIL %u\n",TOTAL,SUCCESS,TOTAL - SUCCESS);
  fclose(output);
}

int main(int argc, char const *argv[]) {
  // Register signal handlers
  signal(SIGINT,logfile);
  signal(SIGQUIT,logfile);
  if (argc != 5) {
    fprintf(stderr,"Usage:./worker fifo_aggregator_to_worker fifo_worker_to_aggregator input_dir bufferSize\n");
    return 1;
  }
  // Read arguments
  string fifo_worker_to_aggregator = (string)argv[2];
  string fifo_aggregator_to_worker = (string)argv[1]; 
  string input_dir = (string) argv[3];
  bufferSize = atoi(argv[4]);
  // Open fifo_aggregator_to_worker to read the country directory names to be opened
  int fifo_aggregator_to_worker_fd = open(fifo_aggregator_to_worker,O_RDONLY);
  // Initialize countries list
  if (!List_Initialize(&countriesList)) {
    close(fifo_aggregator_to_worker_fd);
    return 1;
  }
  // Read the countries sent by diseaseAggregator
  char *countries = receive_data(fifo_aggregator_to_worker_fd,bufferSize,TRUE);
  if (countries != NULL) {
    string country = strtok(countries,"\n");
    while (country != NULL) {
      List_Insert(countriesList,country);
      country = strtok(NULL,"\n");
    }
  }
  // Free memory and fofo needed for receiving the countries
  free(countries);
  close(fifo_aggregator_to_worker_fd);
  // Initialize hashtables
  if (!HashTable_Create(&recordsHT,200,100)) {
    List_Destroy(&countriesList);
    return 1;
  }
  if (!HashTable_Create(&recordsByIdHT,200,100)) {
    HashTable_Destroy(&recordsHT,NULL);
    List_Destroy(&countriesList);
    return 1;
  }
  // Open fifo_worker_to_aggregator to send summary statistics back to the aggregator
  int fifo_worker_to_aggregator_fd = open(fifo_worker_to_aggregator,O_WRONLY);
  // Foreach country read all the files contained in it's folder and save the records
  ListIterator countriesIterator = List_CreateIterator(countriesList);
  while (countriesIterator != NULL) {
    string country = ListIterator_GetValue(countriesIterator);
    DIR *dir_ptr;
    struct dirent *direntp;
    char path[strlen(input_dir) + strlen(country) + 2];
    sprintf(path,"%s/%s",input_dir,country);
    // Open country directory
    if ((dir_ptr = opendir(path)) != NULL) {
      // Read all the files in it
      while ((direntp = readdir(dir_ptr)) != NULL) {
        if (direntp->d_type == DT_REG) {
          string date = direntp->d_name;
          // Open the curent file
          FILE *recordsFile;
          char filePath[strlen(input_dir) + strlen(country) + strlen(date) + 3];
          sprintf(filePath,"%s/%s",path,date);
          if ((recordsFile = fopen(filePath,"r")) == NULL) {
            perror("fopen");
            continue;
          }
          // Maps a disease to a filestat structure to hold summary statistics for that file
          HashTable filestatsHT;
          if (!HashTable_Create(&filestatsHT,200,100)) {
            continue;
          }
          // Read all the lines(records) from the file
          string line = NULL;
          size_t len = 0;
          while (getline(&line,&len,recordsFile) != -1) {
            string recordId = strtok(line," ");
            string type = strtok(NULL," ");
            string patientFirstName = strtok(NULL," ");
            string patientLastName = strtok(NULL," ");
            string disease = strtok(NULL," ");
            int age = atoi(strtok(NULL," "));
            if (!strcmp(type,"ENTER")) {
              // Patient Enters
              // Check if he already entered
              if (HashTable_SearchKey(recordsByIdHT,recordId) == NULL) {
                // Create the record object
                patientRecord record;
                // Insert the record to the id mapped hash table
                if ((record = PatientRecord_Create(recordId,patientFirstName,patientLastName,disease,date,age)) != NULL) {
                  if (HashTable_Insert(recordsByIdHT,recordId,record)) {
                    // Insert the record to the country and disease mapped table
                    // Check if record's disease already exists in that country hash table
                    HashTable diseaseHT;
                    if ((diseaseHT = HashTable_SearchKey(recordsHT,country)) == NULL) {
                      // Not exisits so create a hashtable for that disease
                      if (HashTable_Create(&diseaseHT,200,100)) {
                        HashTable_Insert(recordsHT,country,diseaseHT);
                      }
                    }
                    if (diseaseHT != NULL) {
                      // Chech if disease hash table already contains an avl tree for that disease
                      AvlTree diseaseTree;
                      if ((diseaseTree = HashTable_SearchKey(diseaseHT,disease)) == NULL) {
                        // If not create one
                        if (AvlTree_Create(&diseaseTree)) {
                          // And then insert it to the disease hash table
                          HashTable_Insert(diseaseHT,disease,diseaseTree);
                        }
                      }
                      // Insert the record to the tree
                      if (diseaseTree != NULL) {
                        AvlTree_Insert(diseaseTree,record);
                        // Update statistics for that file
                        filestat *stats;
                        if ((stats = (filestat*)HashTable_SearchKey(filestatsHT,disease)) == NULL) {
                          if ((stats = (filestat*)malloc(sizeof(filestat))) != NULL) {
                            memset(stats,0,sizeof(filestat));
                            if (!HashTable_Insert(filestatsHT,disease,stats)) {
                              free(stats);
                              continue;
                            }
                          }
                        }
                        if (stats != NULL) {
                          if (0 <= age && age <= 20) {
                            stats->years0to20++;
                          } else if (21 <= age && age <= 40) {
                            stats->years21to40++;
                          } else if (41 <= age && age <= 60) {
                            stats->years41to60++;
                          } else {
                            stats->years60plus++;
                          }
                        }
                      }
                    }
                  }
                }
              } else {
                //fprintf(stderr,"ERROR: Patient record %s %s %s %s %s %d tried to enter the hospital twice before exiting the first time.\n",recordId,type,patientFirstName,patientLastName,disease,age);
              }
            } else if (!strcmp(type,"EXIT")) {
              // Patient exits
              patientRecord record;
              // Check if he already entered
              if ((record = HashTable_SearchKey(recordsByIdHT,recordId)) != NULL) {
                // If so mark him as exited
                PatientRecord_Exit(record,date);
              } else {
                //fprintf(stderr,"ERROR: Patient record %s %s %s %s %s %d tried to exit the hospital before entering.\n",recordId,type,patientFirstName,patientLastName,disease,age);
              }
            }
          }
          // Send file's summary statistics to the aggregator
          char headerBuf[strlen(date) + strlen(country) + 2];
          sprintf(headerBuf,"%s\n%s\n",date,country);
          // Send the header
          send_data(fifo_worker_to_aggregator_fd,headerBuf,sizeof(headerBuf),bufferSize);
          HashTable_ExecuteFunctionForAllKeys(filestatsHT,summaryStatistics,1,fifo_worker_to_aggregator_fd);
          free(line);
          fclose(recordsFile);
          // Destroy filestats hash table
          HashTable_ExecuteFunctionForAllKeys(filestatsHT,destroyDiseaseFilestatsFromHT,0);
          HashTable_Destroy(&filestatsHT,NULL);
        }
      }
      // Close countries directory
      closedir(dir_ptr);
    } else {
      fprintf(stderr,"Process %d could not open directory %s\n",getpid(),path);
    }
    ListIterator_MoveToNext(&countriesIterator);
  }
  // Close fifo_worker_to_aggregator_fd
  close(fifo_worker_to_aggregator_fd);
  // Wait for commands from the aggregator
  
  while(TRUE) {
    //printf("---\n");
  }
  printf("FINITO!!!\n");
  // Destroy diseas hash tables in country hash table
  HashTable_ExecuteFunctionForAllKeys(recordsHT,destroyCountryHTdiseaseTables,0);
  // Destroy country hash table
  HashTable_Destroy(&recordsHT,NULL);
  // Destroy records only from id mapped hash table
  HashTable_Destroy(&recordsByIdHT,(int (*)(void**))PatientRecord_Destroy);
  List_Destroy(&countriesList);
  return 0;
}
