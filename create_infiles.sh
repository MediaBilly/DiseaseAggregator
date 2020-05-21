#!/bin/bash

# Read parameters
diseasesFile=$1
countriesFile=$2
input_dir=$3
numFilesPerDirectory=$4
numRecordsPerFile=$5

# Check if diseasesFile exists
if ! [ -f "$diseasesFile" ]; then
  echo No such diseasesFile $diseasesFile 
  exit 1
fi

# Check if countriesFile exists
if ! [ -f "$countriesFile" ]; then
  echo No such countriesFile $countriesFile 
  exit 1
fi

# Number check
if ! [ "$numFilesPerDirectory" -gt 0 ] || ! [ "$numRecordsPerFile" -gt 0 ]; then 
  echo numbers are not ok
  exit 1
fi

# Check if diseasesFile is empty
if ! [ -s "$diseasesFile" ]; then
  echo $diseasesFile is empty
  exit 1
fi

# Check if countriesFile is empty
if ! [ -s "$countriesFile" ]; then
  echo $countriesFile is empty
  exit 1
fi

# Check if input_dir not exists
if [ -e "$input_dir" ]; then
  echo $input_dir already exists
  exit 1
fi

# Read diseases from diseasesFile
diseases=($(cat "$diseasesFile"))

# Read countries from countriesFile
countries=($(cat "$countriesFile"))

# Define first_names array
declare -a first_names=("John" "William" "James" "George" "Henry" "Thomas" "Charles" "Joseph" "Samuel" "David" "Mary" "Sarah" "Elizabeth" "Martha" "Margaret" "Nancy" "Ann" "Jane" "Eliza" "Catherine")

# Define last_names array
declare -a last_names=("Smith" "Brown" "Miller" "Johnson" "Jones" "Davis" "Williams" "Wilson" "Clark" "Taylor")

# Define record types (ENTER and EXIT)
declare -a recordTypes=("ENTER" "EXIT")

# Create input_dir
mkdir $input_dir

# Go to the input_dir
cd $input_dir

# Initialize recordId iterator
recordId=0

# Loop through all the countries
for country in $(seq 0 $((${#countries[@]}  - 1)))
do
  # Create a directory in input_dir for the current country
  mkdir ${countries[$country]}
  # Go inside that directory
  cd ${countries[$country]}
  # Create numFilesPerDirectory files inside that directory
  for fileIndex in $(seq 1 ${numFilesPerDirectory})
  do
    # The new file's name is a random date of format DD-MM-YYYY
    day=($(expr $RANDOM % 30 + 1))
    month=($(expr $RANDOM % 12 + 1))
    year=($(expr $RANDOM % 21 + 2000))
    filename=($day-$month-$year)
    touch $filename
    # Add numRecordsPerFile records in that file
    for recordIndex in $(seq 1 ${numRecordsPerFile})
    do
      # 8/10 records are ENTER
      if [ "$recordIndex" -eq "1" ] || [ "$(expr $RANDOM % 10)" -le "8" ]; then
        # If record decided to be ENTER type create a new one
        recordType=${recordTypes[0]}
        patientFirstName=${first_names[($(expr $RANDOM % ${#first_names[@]}))]}
        patientLastName=${last_names[($(expr $RANDOM % ${#last_names[@]}))]}
        disease=${diseases[($(expr $RANDOM % ${#diseases[@]}))]}
        age=($(expr $RANDOM % 120 + 1))
        echo $recordId $recordType $patientFirstName $patientLastName $disease $age >> $filename
        recordId=$(($recordId+1))
      else
        # If record decided to be EXIT choose one from the already created ones and mark it as exit
        randomFile=($(ls | shuf -n 1))
        # Get random record from the selected file
        randomRecord=($(shuf $randomFile -n 1))
        randomRecord=${randomRecord[*]}
        echo ${randomRecord/ENTER/EXIT} >> $filename
      fi
    done
  done
  # Go back 
  cd ..
done
# Go back 
cd ..