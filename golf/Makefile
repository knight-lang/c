CC=gcc
override CFLAGS+=-Wall -Wpedantic -Wextra -Werror
ifdef DEBUG
override CFLAGS+=-g -fsanitize=address,undefined
else
CFLAGS=-O1
endif
override CFLAGS+=-Wno-parentheses -Wno-empty-body

.PHONY: clean

all: golf2
golf: golf.o
	$(CC) $(CFLAGS) golf.o -o golf

golf2: golf2.o
	$(CC) $(CFLAGS) golf2.o -o golf2

expanded: expanded.o
	$(CC) $(CFLAGS) expanded.o -o expanded

%.o: %.c
	$(CC) $(CFLAGS) -c $<

clean:
	-@rm *.o expanded golf
