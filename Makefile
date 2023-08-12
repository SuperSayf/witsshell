CC = gcc
CFLAGS = -Wall -Wextra
SRC_DIR = src

TARGET = witsshell

all: $(TARGET)

$(TARGET): $(SRC_DIR)/witsshell.c
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC_DIR)/witsshell.c

clean:
	rm -f $(TARGET)
