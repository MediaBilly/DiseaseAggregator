CC = gcc
FLAGS = -Wall -g3
TARGETS = diseaseAggregator worker
OBJS = diseaseAggregator.o utils.o hashtable.o worker.o list.o
SRC_DIR = ./src

all:$(TARGETS)

diseaseAggregator:diseaseAggregator.o utils.o hashtable.o list.o
	$(CC) $(FLAGS) -o diseaseAggregator diseaseAggregator.o hashtable.o utils.o list.o

worker:worker.o utils.o
	$(CC) $(FLAGS) -o worker worker.o utils.o

diseaseAggregator.o:$(SRC_DIR)/diseaseAggregator.c
	$(CC) $(FLAGS) -o diseaseAggregator.o -c $(SRC_DIR)/diseaseAggregator.c

hashtable.o:$(SRC_DIR)/hashtable.c
	$(CC) $(FLAGS) -o hashtable.o -c $(SRC_DIR)/hashtable.c

list.o:$(SRC_DIR)/list.c
	$(CC) $(FLAGS) -o list.o -c $(SRC_DIR)/list.c

worker.o:$(SRC_DIR)/worker.c
	$(CC) $(FLAGS) -o worker.o -c $(SRC_DIR)/worker.c

utils.o:$(SRC_DIR)/utils.c
	$(CC) $(FLAGS) -o utils.o -c $(SRC_DIR)/utils.c

.PHONY : clean

clean:
	rm -f $(TARGETS) $(OBJS)