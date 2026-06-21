CC=gcc
CFLAGS=-Wall -Wextra -O2 -pthread `pkg-config --cflags gtk+-3.0`
LDFLAGS=-pthread `pkg-config --libs gtk+-3.0`
TARGET=bin/process_file_socket_gtk
SRC=src/main.c src/process_utils.c src/file_syscall.c src/socket_demo.c src/network_info.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p bin
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

run: all
	./$(TARGET)

clean:
	rm -rf bin
