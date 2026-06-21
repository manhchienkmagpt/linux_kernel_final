CC=gcc
CFLAGS=-Wall -Wextra -O2 `pkg-config --cflags gtk4`
LDFLAGS=`pkg-config --libs gtk4`
TARGET=system_admin_gui
SRC=src/main.c \
    src/ui_main_window.c \
    src/ui_file_page.c \
    src/ui_cron_page.c \
    src/ui_time_page.c \
    src/ui_apt_page.c \
    src/ui_log_page.c \
    src/system_commands.c

.PHONY: all run clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

run: all
	./$(TARGET)

clean:
	rm -f $(TARGET)
