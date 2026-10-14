CC = cc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic -Iinclude
TARGET = myls
SOURCES = src/main.c src/options.c src/directory.c
OBJECTS = $(SOURCES:.c=.o)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $(OBJECTS)

src/%.o: src/%.c include/ls.h include/options.h include/directory.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(TARGET)
