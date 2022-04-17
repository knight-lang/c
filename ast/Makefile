CC?=gcc
CFLAGS+=-Wall -Wextra -Wpedantic -std=c11
SRCDIR?=src
OBJDIR?=obj
BINDIR?=bin

exe=$(BINDIR)/knight
dyn=$(BINDIR)/libknight.so
source_files=$(wildcard $(SRCDIR)/*.c)
objects=$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(source_files))

KN_FLAGS?=-DKN_AST_CACHE -DKN_STRING_CACHE

override CFLAGS+=-F$(SRCDIR)

ifdef DEBUG
override CFLAGS+=-g -fsanitize=address,undefined
else
override CFLAGS+=-O3 -flto -DNDEBUG
endif

ifdef COMPUTED_GOTOS
override CFLAGS+=-DKN_COMPUTED_GOTOS -Wno-gnu-label-as-value -Wno-gnu-designator
endif

override CFLAGS+=$(KN_FLAGS)

.PHONY: all optimized clean shared

all: $(exe)
shared: $(dyn)

clean:
	-@rm -r $(OBJDIR) $(BINDIR)

optimized:
	$(CC) $(CFLAGS) -o $(exe) $(source_files)

$(exe): $(objects) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $+

$(dyn): $(objects) | $(BINDIR)
	$(CC) $(CFLAGS) -shared -o $@ $+

$(BINDIR):
	@mkdir -p $(BINDIR)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(objects): | $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
