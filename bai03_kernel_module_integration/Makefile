CC=gcc
CFLAGS=-Wall -Wextra -O2 `pkg-config --cflags gtk+-3.0`
LDFLAGS=`pkg-config --libs gtk+-3.0`
KDIR ?= /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
APP=bin/kernel_module_gtk

.PHONY: all app module run clean

all: app module

app: $(APP)

$(APP): src/main.c
	mkdir -p bin
	$(CC) $(CFLAGS) -o $(APP) src/main.c $(LDFLAGS)

module:
	$(MAKE) -C $(KDIR) M=$(PWD)/src modules

run: app
	./$(APP)

clean:
	rm -rf bin
	$(MAKE) -C $(KDIR) M=$(PWD)/src clean
