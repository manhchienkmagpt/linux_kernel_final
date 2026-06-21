CC=gcc
CFLAGS=-Wall -Wextra -O2 -pthread `pkg-config --cflags gtk4`
LDFLAGS=-pthread `pkg-config --libs gtk4`
TARGET=bin/process_file_socket_gui
SRC=src/main.c \
    src/ui_main_window.c \
    src/ui_dashboard_page.c \
    src/ui_process_page.c \
    src/ui_file_page.c \
    src/ui_socket_page.c \
    src/ui_network_page.c \
    src/ui_log_page.c \
    src/process_utils.c \
    src/file_syscall.c \
    src/socket_demo.c \
    src/network_info.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	mkdir -p bin
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

run: all
	./$(TARGET)

clean:
	rm -rf bin
