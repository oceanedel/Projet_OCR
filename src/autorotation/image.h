#ifndef IMAGE_H
#define IMAGE_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#define PI 3.14159265

typedef struct pPixel {
  Uint8 r, g, b;
  struct pPixel *matrix;
} pPixel;

typedef struct iImage {
  int height, width;
  char *path;
  pPixel **pixels;
  int label;
} iImage;

int init_SDL();

SDL_Surface *load_surface(const char *image_path);

iImage *create_image(unsigned int width, unsigned int height,
                     const char *image_path);

void extract_pixels(SDL_Surface *surface, iImage *img);

iImage *load_image(const char *image_path, int label);

void free_image(iImage *image_path);

void save_image(iImage *img, const char *image_path);

iImage *create_subimage(const iImage *original, unsigned int x, unsigned int y,
                        unsigned int width, unsigned int height);

#endif // IMAGE_H
