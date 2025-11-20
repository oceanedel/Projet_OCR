# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -D_POSIX_C_SOURCE=200809L `sdl2-config --cflags`
GTK_FLAGS = `pkg-config --cflags gtk+-3.0`
LDFLAGS = `sdl2-config --libs` -lm -lSDL2 -lSDL2_image
GTK_LDFLAGS = `pkg-config --libs gtk+-3.0` -lm 

# Directories
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
BIN_DIR = $(BUILD_DIR)/bin

# Targets
MAIN_TARGET = $(BIN_DIR)/wordsearch_solver
GUI_TARGET = $(BIN_DIR)/image_rotator

# Source files
EXTRACTION_SRC = $(wildcard src/extraction/*.c)
SOLVER_SRC = src/solver/solver.c
OCR_SRC = $(wildcard src/ocr/*.c)
MAIN_SRC = main.c
GUI_SRC = src/gui/main.c

# Object files
EXTRACTION_OBJ = $(patsubst src/extraction/%.c, $(OBJ_DIR)/extraction/%.o, $(EXTRACTION_SRC))
SOLVER_OBJ = $(OBJ_DIR)/solver/solver.o
OCR_OBJ = $(patsubst src/ocr/%.c, $(OBJ_DIR)/ocr/%.o, $(OCR_SRC))
MAIN_OBJ = $(OBJ_DIR)/main.o
GUI_OBJ = $(OBJ_DIR)/gui/gui_main.o

# Build everything
all: directories $(MAIN_TARGET) $(GUI_TARGET)
	@echo ""
	@echo "✓ Build complete!"
	@echo "  Main solver: $(MAIN_TARGET)"
	@echo "  GUI rotator: $(GUI_TARGET)"

# Create build directories
directories:
	@mkdir -p $(OBJ_DIR)/extraction
	@mkdir -p $(OBJ_DIR)/solver
	@mkdir -p $(OBJ_DIR)/ocr
	@mkdir -p $(OBJ_DIR)/gui
	@mkdir -p $(BIN_DIR)
	@mkdir -p output/cells
	@mkdir -p output/words
	@mkdir -p training

# Main solver executable
$(MAIN_TARGET): $(MAIN_OBJ) $(EXTRACTION_OBJ) $(SOLVER_OBJ) $(OCR_OBJ)
	$(CC) $^ $(LDFLAGS) -o $@

# GUI rotation tool
$(GUI_TARGET): $(GUI_OBJ)
	$(CC) $^ $(GTK_LDFLAGS) -o $@

# Compile main.c
$(MAIN_OBJ): $(MAIN_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile extraction modules
$(OBJ_DIR)/extraction/%.o: src/extraction/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile solver
$(SOLVER_OBJ): $(SOLVER_SRC)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile OCR modules
$(OBJ_DIR)/ocr/%.o: src/ocr/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Compile GUI (needs GTK flags)
$(GUI_OBJ): $(GUI_SRC)
	$(CC) $(GTK_FLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	rm -f output/*.bmp output/grid.txt
	rm -rf output/cells/*.bmp
	rm -rf output/words/*.bmp
	rm -rf output/word_letters/*.bmp
	rm -rf output/*.txt
	@echo "✓ Clean complete"

# Run with test image
run: $(MAIN_TARGET)
	./$(MAIN_TARGET) data/test1_1.bmp

# Interactive run
run-interactive: $(MAIN_TARGET)
	./$(MAIN_TARGET)

# Build and run GUI
run-gui: $(GUI_TARGET)
	./$(GUI_TARGET)

clean-output:
	rm -f output/*.bmp output/grid.txt
	rm -rf output/cells/*.bmp
	rm -rf output/words/*.bmp

	
# Help
help:
	@echo "Available targets:"
	@echo "  make           - Build everything"
	@echo "  make run       - Run with test image"
	@echo "  make run-interactive - Run with user input"
	@echo "  make run-gui   - Launch image rotator"
	@echo "  make clean     - Remove build and output files"
	@echo "  make clean-out     - Remove output files"


.PHONY: all directories clean clean-output run run-interactive run-gui help
