CC = gcc
CFLAGS = -Wall -Wextra
LIBS = $(shell pkg-config --cflags --libs libcjson)

TARGET = config_manager

all: $(TARGET)

$(TARGET): config_manager.c
	$(CC) $(CFLAGS) config_manager.c $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET)