CC = gcc
FLAGS = -Wall -g3
TARGETS = diseaseAggregator worker
OBJS = diseaseAggregator.o utils.o hashtable.o worker.o list.o patientRecord.o avltree.o
SRC_DIR = ./src

all:$(TARGETS)

diseaseAggregator:diseaseAggregator.o utils.o hashtable.o list.o
	$(CC) $(FLAGS) -o diseaseAggregator diseaseAggregator.o hashtable.o utils.o list.o

worker:worker.o utils.o list.o patientRecord.o hashtable.o avltree.o
	$(CC) $(FLAGS) -o worker worker.o utils.o list.o patientRecord.o hashtable.o avltree.o

diseaseAggregator.o:$(SRC_DIR)/diseaseAggregator.c
	$(CC) $(FLAGS) -o diseaseAggregator.o -c $(SRC_DIR)/diseaseAggregator.c

hashtable.o:$(SRC_DIR)/hashtable.c
	$(CC) $(FLAGS) -o hashtable.o -c $(SRC_DIR)/hashtable.c

list.o:$(SRC_DIR)/list.c
	$(CC) $(FLAGS) -o list.o -c $(SRC_DIR)/list.c

worker.o:$(SRC_DIR)/worker.c
	$(CC) $(FLAGS) -o worker.o -c $(SRC_DIR)/worker.c

patientRecord.o:$(SRC_DIR)/patientRecord.c
	$(CC) $(FLAGS) -o patientRecord.o -c $(SRC_DIR)/patientRecord.c

avltree.o:$(SRC_DIR)/avltree.c
	$(CC) $(FLAGS) -o avltree.o -c $(SRC_DIR)/avltree.c

utils.o:$(SRC_DIR)/utils.c
	$(CC) $(FLAGS) -o utils.o -c $(SRC_DIR)/utils.c

.PHONY : clean

clean:
	rm -f $(TARGETS) $(OBJS)