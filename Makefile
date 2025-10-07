# Makefile Template (object-file based)

# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -O2 -I. -Ifundamentals -Iproficiencies #-fsanitize=address -g
LDFLAGS := -lm
SDLFLAGS := -lSDL2

# Directories
SRCDIR := src
OBJDIR := obj
BINDIR := bin

# Target executable
TARGET := main

# Source files (relative paths)
SRC := main.c $(SRCDIR)/inkpaper.c

# Object files (map each .c to obj/*.o)
OBJ := $(patsubst %.c,$(OBJDIR)/%.o,$(notdir $(SRC)))

# Default target
all: $(TARGET)

# Link: link object files to produce the final executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(SDLFLAGS)

# Pattern rule: compile each source to corresponding object in $(OBJDIR)
# We compile from either project root or SRCDIR; use % to match basename
$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create directories if missing
$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

# Phony targets and clean
.PHONY: all clean run

clean:
	rm -f $(TARGET) $(OBJDIR)/*.o

run: $(TARGET)
	./$(TARGET)

