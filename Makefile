# Projet_OCR Makefile (Enhanced with Module Targets)
# ===================================================
# Supports: extraction, solver, gui, ocr modules
# Targets: main executable + per-module builds + standalone tools

# ============================
# Compiler and flags
# ============================
CC        = gcc
CFLAGS    = -Wall -Wextra -std=c99 -pedantic -g -O2 -Iinclude
LDFLAGS   = 
LIBS      = -lm `sdl2-config --libs`
SDL_FLAGS = `sdl2-config --cflags`

# ============================
# Directories
# ============================
SRC_DIR       = src
EXTRACT_DIR   = $(SRC_DIR)/extraction
SOLVER_DIR    = $(SRC_DIR)/solver
GUI_DIR       = $(SRC_DIR)/gui
OCR_DIR       = $(SRC_DIR)/ocr
INC_DIR       = include
OBJ_DIR       = build/obj
BIN_DIR       = build/bin
OUTPUT_DIR    = output

# ============================
# Source files
# ============================
EXTRACT_SRC = $(wildcard $(EXTRACT_DIR)/*.c)
SOLVER_SRC  = $(wildcard $(SOLVER_DIR)/*.c)
GUI_SRC     = $(wildcard $(GUI_DIR)/*.c)
OCR_SRC     = $(wildcard $(OCR_DIR)/*.c)

# Object files
EXTRACT_OBJ = $(patsubst $(EXTRACT_DIR)/%.c, $(OBJ_DIR)/extraction/%.o, $(EXTRACT_SRC))
SOLVER_OBJ  = $(patsubst $(SOLVER_DIR)/%.c, $(OBJ_DIR)/solver/%.o, $(SOLVER_SRC))
GUI_OBJ     = $(patsubst $(GUI_DIR)/%.c, $(OBJ_DIR)/gui/%.o, $(GUI_SRC))
OCR_OBJ     = $(patsubst $(OCR_DIR)/%.c, $(OBJ_DIR)/ocr/%.o, $(OCR_SRC))

ALL_OBJ = $(EXTRACT_OBJ) $(SOLVER_OBJ) $(GUI_OBJ) $(OCR_OBJ)

# Main executable
MAIN_SRC = main.c
MAIN_OBJ = $(OBJ_DIR)/main.o
TARGET   = $(BIN_DIR)/ocr_app

# ============================
# Targets
# ============================
.PHONY: all clean dirs help extraction solver gui ocr

all: dirs $(TARGET)

# Build main executable
$(TARGET): $(MAIN_OBJ) $(ALL_OBJ)
	@mkdir -p $(BIN_DIR)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)
	@echo "✓ Built: $(TARGET)"

# ============================
# Per-Module Targets
# ============================

# Build extraction module only
extraction: dirs $(EXTRACT_OBJ)
	@echo "✓ Compiled extraction module"

# Build solver module only
solver: dirs $(SOLVER_OBJ)
	@echo "✓ Compiled solver module"

# Build GUI module only
gui: dirs $(GUI_OBJ)
	@echo "✓ Compiled GUI module"

# Build OCR module only
ocr: dirs $(OCR_OBJ)
	@echo "✓ Compiled OCR module"

# ============================
# Compilation rules
# ============================

# Compile main.c
$(MAIN_OBJ): $(MAIN_SRC)
	@mkdir -p $(OBJ_DIR)
	$(CC) $(CFLAGS) $(SDL_FLAGS) -c $< -o $@

# Compile extraction module
$(OBJ_DIR)/extraction/%.o: $(EXTRACT_DIR)/%.c
	@mkdir -p $(OBJ_DIR)/extraction
	$(CC) $(CFLAGS) $(SDL_FLAGS) -c $< -o $@

# Compile solver module
$(OBJ_DIR)/solver/%.o: $(SOLVER_DIR)/%.c
	@mkdir -p $(OBJ_DIR)/solver
	$(CC) $(CFLAGS) -c $< -o $@

# Compile GUI module
$(OBJ_DIR)/gui/%.o: $(GUI_DIR)/%.c
	@mkdir -p $(OBJ_DIR)/gui
	$(CC) $(CFLAGS) $(SDL_FLAGS) -c $< -o $@

# Compile OCR module
$(OBJ_DIR)/ocr/%.o: $(OCR_DIR)/%.c
	@mkdir -p $(OBJ_DIR)/ocr
	$(CC) $(CFLAGS) $(SDL_FLAGS) -c $< -o $@

# ============================
# Optional: Standalone tools
# ============================

# Example: standalone extraction tool (if you have a main in extraction)
# Uncomment and adapt when ready:
# extract_tool: dirs $(EXTRACT_OBJ)
# 	$(CC) $(LDFLAGS) -o $(BIN_DIR)/extract_tool $(EXTRACT_OBJ) $(LIBS)
# 	@echo "✓ Built standalone: extract_tool"

# ============================
# Directory setup
# ============================
dirs:
	@mkdir -p $(OBJ_DIR)/extraction $(OBJ_DIR)/solver $(OBJ_DIR)/gui $(OBJ_DIR)/ocr
	@mkdir -p $(BIN_DIR)
	@mkdir -p $(OUTPUT_DIR)/grids $(OUTPUT_DIR)/words $(OUTPUT_DIR)/cells $(OUTPUT_DIR)/debug

# ============================
# Clean build artifacts
# ============================
clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)
	rm -f output/cells/* output/grids/* output/words/*
	@echo "✓ Cleaned build artifacts"

clean-all: clean
	rm -rf $(OUTPUT_DIR)/*
	@echo "✓ Cleaned output directory"

clean-out:
	rm -f output/*bmp output/cells/* output/grids/* output/words/*
	@echo "✓ Cleaned outputs"

# ============================
# Help
# ============================
help:
	@echo "Projet_OCR Makefile"
	@echo "==================="
	@echo "Targets:"
	@echo "  all        - Build the full application (default)"
	@echo "  extraction - Compile extraction module only"
	@echo "  solver     - Compile solver module only"
	@echo "  gui        - Compile GUI module only"
	@echo "  ocr        - Compile OCR module only"
	@echo "  clean      - Remove build artifacts"
	@echo "  clean-all  - Remove build artifacts + outputs"
	@echo "  dirs       - Create necessary directories"
	@echo "  help       - Show this help"
	@echo ""
	@echo "Usage:"
	@echo "  make                # Build everything"
	@echo "  make extraction     # Compile extraction module"
	@echo "  make clean && make  # Clean rebuild"
