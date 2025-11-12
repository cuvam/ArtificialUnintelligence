CC = gcc
CFLAGS = -Wall -Wextra -O2 -lm
DEPS = matrix.h neural_network.h
OBJ = matrix.o neural_network.o

all: xor_example regression_example

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

xor_example: xor_example.o $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

regression_example: regression_example.o $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)

clean:
	rm -f *.o xor_example regression_example

.PHONY: all clean
