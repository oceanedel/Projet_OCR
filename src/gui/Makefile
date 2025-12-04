CC = gcc
CFLAGS = -Wall -O2 $(shell pkg-config --cflags gtk+-3.0 sdl2 SDL2_image)
LDLIBS = $(shell pkg-config --libs gtk+-3.0 sdl2 SDL2_image) -lm


TARGET = image_rotator

SRC = main.c \
      ../autorotation/rotation.c \
      ../autorotation/image.c \
      ../extraction/preprocess.c \
      ../extraction/extract_grid.c \
      ../extraction/slice_grid.c \
      ../extraction/extract_wordlist.c \
      ../extraction/slice_words.c \
      ../extraction/slice_letter_word.c \
      ../extraction/trim_cells.c \
      ../extraction/trim_word_letters.c \
      ../solver/solver.c \
      ../ocr/letter_recognition.c \
      ../ocr/grid_processor.c \
      ../ocr/word_processor.c

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDLIBS)

clean:
	rm -f $(OBJ) $(TARGET)
	rm -f ../../output/*.bmp ../../output/grid.txt
	rm -rf ../../output/cells/*.bmp
	rm -rf ../../output/words/*.bmp
	rm -rf ../../output/word_letters/*.bmp
	rm -rf ../../output/*.txt
	rm -rf ../../training
	@echo "✓ Clean complete"
