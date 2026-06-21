CC=gcc
CFLAGS=-Wall -Wextra -O2 `pkg-config --cflags gtk4`
LDFLAGS=`pkg-config --libs gtk4`
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
APP=bin/kernel_module_gtk
APP_SRC=src/main.c \
	src/ui_main_window.c \
	src/ui_dashboard_page.c \
	src/ui_module_control_page.c \
	src/ui_device_io_page.c \
	src/ui_command_console_page.c \
	src/ui_kernel_log_page.c \
	src/ui_help_page.c \
	src/kernel_module_commands.c

.PHONY: all app module run clean

all: app module

app: $(APP)

$(APP): $(APP_SRC)
	mkdir -p bin
	$(CC) $(CFLAGS) -o $(APP) $(APP_SRC) $(LDFLAGS)

module:
	$(MAKE) -C $(KDIR) M=$(PWD)/src modules

run: app
	./$(APP)

clean:
	rm -rf bin
	$(MAKE) -C $(KDIR) M=$(PWD)/src clean
