
CFLAGS=-ggdb3  #-fsanitize=address

all:
	gcc $(CFLAGS) -c jsonrpc.c -fPIC `pkg-config --cflags geany` -Wall -Werror -Wswitch
	gcc $(CFLAGS) -c -DGEANY lsp.c -fPIC `pkg-config --cflags geany json-c`  -Wall -Werror
	gcc $(CFLAGS) -c plugin.c -fPIC `pkg-config --cflags geany` -Wall -Werror -Wswitch
	gcc $(CFLAGS) plugin.o lsp.o jsonrpc.o -o copilot.geany.so -shared `pkg-config --libs geany json-c` -lbsd
	
	#cp plugin.so ~/.config/geany/plugins/
	sudo cp copilot.geany.so /usr/lib/x86_64-linux-gnu/geany
	sudo cp copilot.geany.so /usr/local/lib/geany/
