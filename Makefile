SHELL=/bin/bash

CFLAGS=-ggdb3 -fPIC `pkg-config --cflags geany` -Wall -Werror -Wswitch -MMD  #-fsanitize=address

BUILD_DIR = build
SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))
DEPS = $(OBJS:.o=.d)
	

all: 

	mkdir -p build
	
	python gen_engines.py
	
	$(MAKE) lib

	
	#cp plugin.so ~/.config/geany/plugins/
	sudo cp copilot.geany.so /usr/lib/x86_64-linux-gnu/geany
	sudo cp copilot.geany.so /usr/local/lib/geany/


-include $(DEPS)


$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@
	
	
	
lib: $(OBJS)

	gcc $(CFLAGS) $(OBJS) -o copilot.geany.so -shared `pkg-config --libs geany json-c` -lbsd
