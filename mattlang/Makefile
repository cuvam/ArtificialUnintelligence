CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
TARGET = matt
SOURCES = main.c lexer.c parser.c interpreter.c utils.c
OBJECTS = $(SOURCES:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

%.o: %.c matt.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)

test: $(TARGET)
	@echo "Running tests..."
	@for test_file in tests/*.matt; do \
		echo "Testing $$test_file..."; \
		./$(TARGET) $$test_file; \
		echo ""; \
	done

.PHONY: all clean test
