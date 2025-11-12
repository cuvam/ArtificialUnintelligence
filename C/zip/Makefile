CC = gcc
CFLAGS = -Wall -Wextra -O2

TARGET = zip_util
SRC = zip_util.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

install:
	cp $(TARGET) /usr/local/bin/

.PHONY: all clean install
