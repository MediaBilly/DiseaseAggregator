#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "../headers/patientRecord.h"
#include "../headers/utils.h"

struct patient_record {
  string recordID;
  string patientFirstName;
  string patientLastName;
  string disease;
  time_t entryDate;
  time_t exitDate;
  int age;
  boolean exited;
};

patientRecord PatientRecord_Create(string recordID,string patientFirstName,string patientLastName,string disease,string entryDate,int age) {
  patientRecord record;
  // Allocate space for patientRecord object
  if ((record = (patientRecord)malloc(sizeof(struct patient_record))) == NULL) {
    not_enough_memory();
    return NULL;
  }
  // Copy record contents
  if ((record->recordID = CopyString(recordID)) == NULL) {
    not_enough_memory();
    PatientRecord_Destroy(&record);
    return NULL;
  }
  if ((record->patientFirstName = CopyString(patientFirstName)) == NULL) {
    not_enough_memory();
    PatientRecord_Destroy(&record);
    return NULL;
  }
  if ((record->patientLastName = CopyString(patientLastName)) == NULL) {
    not_enough_memory();
    PatientRecord_Destroy(&record);
    return NULL;
  }
  if ((record->disease = CopyString(disease)) == NULL) {
    not_enough_memory();
    PatientRecord_Destroy(&record);
    return NULL;
  }
  struct tm tmpTime;
  memset(&tmpTime,0,sizeof(struct tm));
  if (strptime(entryDate,"%d-%m-%Y",&tmpTime) == NULL) {
    printf("entryDate %s parsing failed!\n",entryDate);
    return NULL;
  } else {
    record->entryDate = mktime(&tmpTime);
  }
  record->age = age;
  //printf("Record added\n");
  return record;
}

int PatientRecord_Exited(patientRecord record) {
  return record->exited;
}

string PatientRecord_Get_recordID(patientRecord record) {
  return record->recordID;
}

string PatientRecord_Get_disease(patientRecord record) {
  return record->disease;
}

time_t PatientRecord_Get_entryDate(patientRecord record) {
  return record->entryDate;
}

time_t PatientRecord_Get_exitDate(patientRecord record) {
  return record->exited ? record->exitDate : 0;
}

int PatientRecord_Exit(patientRecord record,string exitDateStr) {
  struct tm tmpTime;
  memset(&tmpTime,0,sizeof(struct tm));
  time_t exitDate;
  if (strptime(exitDateStr,"%d-%m-%Y",&tmpTime) != NULL) {
    if (difftime((exitDate = mktime(&tmpTime)),record->entryDate) >= 0) {
      record->exitDate = exitDate;
      record->exited = TRUE;
      //printf("Record updated\n");
      return TRUE;
    } else {
      //printf("The exitDate of the record with id %s is earlier than it's entryDate. Ignoring update.\n",record->recordID);
      return FALSE;
    }
  } else {
    printf("Wrong date format.\n");
    return FALSE;
  }
}

int PatientRecord_Destroy(patientRecord *record) {
  if (*record != NULL) {
    // Destroy the strings first
    DestroyString(&(*record)->recordID);
    DestroyString(&(*record)->patientFirstName);
    DestroyString(&(*record)->patientLastName);
    DestroyString(&(*record)->disease);
    // Then the record itself
    free(*record);
    *record = NULL;
    return TRUE;
  } else {
    return FALSE;
  }
}