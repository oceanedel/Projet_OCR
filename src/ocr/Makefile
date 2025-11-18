CC ?= gcc
CFLAGS ?=  -O2 -Wall -Wextra -Werror
LDLIBS ?= -lm -lSDL2 -lSDL2_image

SRC = letter_recognition.c
BIN = letter_recognition

.PHONY: all clean

all: $(BIN)

$(BIN): $(SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f $(BIN) bH.txt bO.txt wHO.txt wIH.txt
