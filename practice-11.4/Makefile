CC = gcc
CFLAGS = -Wall -Wextra
LIBS = $(shell pkg-config --cflags --libs libcurl sqlite3 libcjson)

TARGET = weather

all: $(TARGET)

$(TARGET): weather.c http.c db.c
	$(CC) $(CFLAGS) weather.c http.c db.c $(LIBS) -o $(TARGET)

clean:
	rm -f $(TARGET) weather.db

run: $(TARGET)
	./$(TARGET) fetch Moscow

history: $(TARGET)
	./$(TARGET) history

last: $(TARGET)
	./$(TARGET) last

.PHONY: all clean run history last