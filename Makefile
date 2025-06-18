CC = gcc
CFLAGS = -Wall -Wextra -std=c99
TARGET = battleships
SOURCE = new.c

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCE)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run 
