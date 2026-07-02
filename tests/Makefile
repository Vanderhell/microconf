CC ?= cc
CPPFLAGS ?=
CFLAGS ?=
LDFLAGS ?=

CPPFLAGS += -I../include
CFLAGS += -std=c99 -Wall -Wextra -Wpedantic

SRC = ../src/mconf.c
TEST = test_all.c
BIN = test_all

.PHONY: all clean

all: $(BIN)
	./$(BIN)

$(BIN): $(SRC) $(TEST) ../include/mconf.h ../include/mconf_config.h
	$(CC) $(CPPFLAGS) $(CFLAGS) $(SRC) $(TEST) $(LDFLAGS) -o $@

clean:
	rm -f $(BIN)
