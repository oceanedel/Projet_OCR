CC = gcc
# 1. On sépare les flags de compilation (--cflags) ...
# On utilise $(shell ...) pour que make exécute la commande proprement.
CFLAGS = -Wall -O2 $(shell pkg-config --cflags gtk+-3.0 sdl2 SDL2_image)

# 2. ... des flags de lien (--libs)
LDLIBS = $(shell pkg-config --libs gtk+-3.0 sdl2 SDL2_image) -lm

TARGET = image_rotator

SRC = \
    main.c \
    ../autorotation/rotation.c \
    ../autorotation/image.c \

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	# Ici, on utilise $(LDLIBS) pour lier les bibliothèques
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ) $(LDLIBS)

# Règle optionnelle mais recommandée pour forcer la recompilation si les .h changent
# (Pour l'instant, la règle implicite de Make suffira grâce au CFLAGS corrigé)

clean:
	rm -f $(OBJ) $(TARGET)

