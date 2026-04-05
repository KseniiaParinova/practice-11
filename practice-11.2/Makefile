CC = gcc
CFLAGS = -Wall -Wextra
LIBS = $(shell pkg-config --cflags --libs libcurl)

TARGET = http_client

all: $(TARGET)

$(TARGET): http_client.c
	$(CC) $(CFLAGS) http_client.c $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	./$(TARGET) get https://httpbin.org/get