CC = gcc
PKG = `pkg-config --cflags --libs gtk+-3.0`
CFLAGS = -Wall -O2
TARGET = image_rotator
SRC = main.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(PKG) -lm

clean:
	rm -f $(TARGET)
