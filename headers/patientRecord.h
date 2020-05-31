#ifndef PATIENT_RECORD_H
#define PATIENT_RECORD_H

#include <time.h>
#include "./utils.h"

typedef struct patient_record *patientRecord;

// Creates a patientRecord object
patientRecord PatientRecord_Create(string,string,string,string,string,int);
// Returns TRUE if patientRecord's patient exited the hospital and FALSE otherwise
int PatientRecord_Exited(patientRecord);
// Getters
string PatientRecord_Get_recordID(patientRecord);
string PatientRecord_Get_disease(patientRecord);
int PatientRecord_Get_age(patientRecord);
time_t PatientRecord_Get_entryDate(patientRecord);
time_t PatientRecord_Get_exitDate(patientRecord);
string PatientRecord_ToString(patientRecord);
// Exit date updater
int PatientRecord_Exit(patientRecord,string);
// Destroys a patientRecord object
int PatientRecord_Destroy(patientRecord*);

#endif