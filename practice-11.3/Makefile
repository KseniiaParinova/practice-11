CC = gcc
CFLAGS = -Wall -Wextra
LIBS = $(shell pkg-config --cflags --libs sqlite3)

TARGET = todo

all: $(TARGET)

$(TARGET): todo.c
	$(CC) $(CFLAGS) todo.c $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET) todo.db

run: $(TARGET)
	./$(TARGET) list