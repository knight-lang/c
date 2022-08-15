CC=gcc
override CFLAGS+=-Wall -Wpedantic -Wextra -Werror
ifdef DEBUG
override CFLAGS+=-g -fsanitize=address,undefined
else
CFLAGS=-O1
endif

.PHONY: clean

golf: golf.o
	$(CC) $(CFLAGS) golf.o -o golf

expanded: expanded.o
	$(CC) $(CFLAGS) expanded.o -o expanded

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	-@rm *.o expanded golf
