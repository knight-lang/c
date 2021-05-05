SRCDIR?=src
OBJDIR?=obj
EXE?=knight

CC=gcc
CFLAGS+=-Wall -Wextra -Wpedantic -I../ast
LDFLAGS=-lknight -L../ast/bin

override CFLAGS += -F$(SRCDIR)

ifdef DEBUG
override CFLAGS+=-g -fsanitize=address,undefined
endif

ifdef OPTIMIZED
CEXEFLAGS+=-O3 -DNDEBUG -flto -march=native -Wno-gnu
endif

objects = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(wildcard $(SRCDIR)/*.c))

.PHONY: optimized clean

all: $(EXE)

optimized:
	$(CC) $(CFLAGS) $(LDFLAGS) $(CEXEFLAGS) -o $(EXE) $(wildcard $(SRCDIR)/*.c)

$(EXE): $(objects)
	$(CC) $(CFLAGS) $(LDFLAGS) $(CEXEFLAGS) -o $@ $+

clean:
	-@rm -r $(OBJDIR)
	-@rm $(EXE)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(objects): | $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
