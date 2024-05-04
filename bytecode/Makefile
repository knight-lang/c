CC?=gcc
CFLAGS+=-Wall -Wextra -Wpedantic -std=c11
SRCDIR?=src
INCLDIR?=include/knight
OBJDIR?=obj
BINDIR?=bin
CORPUSDIR?=corpus

exe=$(BINDIR)/knight
dyn=$(BINDIR)/libknight.so
source_files=$(wildcard $(SRCDIR)/*.c)
objects=$(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(source_files))

KN_FLAGS?=-DKN_AST_CACHE -DKN_STRING_CACHE # -DKN_USE_REFCOUNT

override CFLAGS+=-I$(INCLDIR) -F$(SRCDIR)

ifeq ($(MAKECMDGOALS),fuzzer) 
override CFLAGS+=-g -fsanitize=address,undefined,fuzzer -DKN_FUZZING
else ifeq ($(MAKECMDGOALS),debug)
override CFLAGS+=-g -fsanitize=address,undefined
else
override CFLAGS+=-O2 -flto -DNDEBUG
endif

ifdef COMPUTED_GOTOS
override CFLAGS+=-DKN_COMPUTED_GOTOS -Wno-gnu-label-as-value -Wno-gnu-designator
endif

override CFLAGS+=$(KN_FLAGS)

.PHONY: all optimized clean shared fuzzer

all: $(exe)
debug: $(exe)
shared: $(dyn)

fuzzer: $(exe) $(CORPUSDIR)
	@echo "0" > $(CORPUSDIR)/initial.txt

clean:
	-@rm -r $(OBJDIR) $(BINDIR) $(CORPUSDIR)

optimized:
	$(CC) $(CFLAGS) -o $(exe) $(source_files)

$(exe): $(objects) | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $+

$(dyn): $(objects) | $(BINDIR)
	$(CC) $(CFLAGS) -shared -o $@ $+

$(BINDIR):
	@mkdir -p $(BINDIR)

$(CORPUSDIR):
	@mkdir -p $(CORPUSDIR)

$(OBJDIR):
	@mkdir -p $(OBJDIR)

$(objects): | $(OBJDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@
