CC = gcc
PKG = `pkg-config --cflags --libs gtk+-3.0`
CFLAGS = -Wall -O2
TARGET = image_rotator

SRC = \
    main.c \
    autorotation/rotation.c \
    autorotation/image.c \

OBJ = $(SRC:.c=.o)

PKG = `pkg-config --cflags --libs gtk+-3.0 sdl2 SDL2_image`

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(PKG) -lm

clean:
	rm -f $(OBJ) $(TARGET)

