CC = gcc
CFLAGS = -Wall -Wextra -O2 $(shell pkg-config --cflags gtk+-3.0 sdl2 SDL2_image)
LDLIBS = $(shell pkg-config --libs gtk+-3.0 sdl2 SDL2_image) -lm


TARGET = projet_ocr

SRC = src/gui/main.c \
      src/autorotation/rotation.c \
      src/autorotation/image.c \
      src/extraction/preprocess.c \
      src/extraction/extract_grid.c \
      src/extraction/slice_grid.c \
      src/extraction/extract_wordlist.c \
      src/extraction/slice_words.c \
      src/extraction/slice_letter_word.c \
      src/extraction/trim_cells.c \
	  src/extraction/denoiser.c\
      src/extraction/trim_word_letters.c \
      src/solver/solver.c \
      src/ocr/letter_recognition.c \
      src/ocr/grid_processor.c \
      src/ocr/word_processor.c \
	  src/result/result.c \
	  src/extraction/slice_grid_no_lines.c
		

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDLIBS)

clean:
	rm -f $(OBJ) $(TARGET)
	rm -f ./output/*.bmp ./output/grid.txt
	rm -rf ./output/cells/*.bmp
	rm -rf ./output/words/*.bmp
	rm -rf ./output/word_letters/*.bmp
	rm -rf ./output/words.txt
	rm -rf ./output/grid.txt
	@echo "✓ Clean complete"

clean-training:
	rm -rf ./output/*.txt
