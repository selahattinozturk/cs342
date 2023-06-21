CC = gcc
CFLAGS = -Wall -Wextra

TARGET = pvm

all: $(TARGET)

$(TARGET): pvm.c
	$(CC) $(CFLAGS) -o $(TARGET) pvm.c

clean:
	rm -f $(TARGET)