CC = gcc
FLAGS = -Wall
TARGETS = diseaseAggregator 
OBJS = diseaseAggregator.o utils.o
SRC_DIR = ./src

all:$(TARGETS)

diseaseAggregator:diseaseAggregator.o utils.o
	$(CC) $(FLAGS) -o diseaseAggregator diseaseAggregator.o utils.o

diseaseAggregator.o:$(SRC_DIR)/diseaseAggregator.c
	$(CC) $(FLAGS) -o diseaseAggregator.o -c $(SRC_DIR)/diseaseAggregator.c

utils.o:$(SRC_DIR)/utils.c
	$(CC) $(FLAGS) -o utils.o -c $(SRC_DIR)/utils.c

.PHONY : clean

clean:
	rm -f $(TARGETS) $(OBJS)