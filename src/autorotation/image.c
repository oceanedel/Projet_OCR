#include <SDL2/SDL.h>

#include <SDL2/SDL_image.h>

#include <err.h>

#include <stdio.h>

#include <stdlib.h>

#include <string.h>



#include "image.h"



int init_SDL() 

{

  if (SDL_Init(SDL_INIT_VIDEO) != 0) 

  {

    err(EXIT_FAILURE, "error while init");

  }



  if (!(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG))

  {

    err(EXIT_FAILURE, "error while init");

  }



  return EXIT_SUCCESS;

}



/*

    Loads a surface from a path using SDL2

*/

SDL_Surface *load_surface(const char *image_path)

{

  SDL_Surface *surface = IMG_Load(image_path);

  if (surface == NULL) 

  {

    err(EXIT_FAILURE, "error while loading img");

  }

  return surface;

}



/*

    Creates an image from a path

*/

iImage *create_image(unsigned int width, unsigned int height,

                     const char *image_path) 

{

  iImage *img = (iImage *)malloc(sizeof(iImage));

  if (img == NULL) 

  {

    printf("An error occured while allocating memory.\n");

    return NULL;

  }



  img->width = width;

  img->height = height;



  img->path = (char *)malloc(strlen(image_path) + 1);

  if (img->path == NULL) 

  {

    printf("An error occured while allocating memory for the path's data.\n");

    free(img);

    return NULL;

  }

  strcpy(img->path, image_path);



  img->pixels = (pPixel **)malloc(height * sizeof(pPixel *));

  if (img->pixels == NULL) {

    printf("An error occured while allocating memory for the pixels's "

           "data. \n");

    free(img->path);

    free(img);

    return NULL;

  }



  for (unsigned int i = 0; i < height; ++i) 

  {

    img->pixels[i] = (pPixel *)malloc(width * sizeof(pPixel));

    if (img->pixels[i] == NULL) 

    {

      printf("An error occured while allocating memory for the pixels's "

             "line, %d\n",

             i);

      for (unsigned int j = 0; j < i; ++j) 

      {

        free(img->pixels[j]);

      }

      free(img->pixels);

      free(img->path);

      free(img);

      return NULL;

    }

  }



  return img;

}



/*

    Extracts pixels from SDL surface and copy them in a iImage struct

*/

void extract_pixels(SDL_Surface *surface, iImage *img) 

{

  SDL_LockSurface(surface);



  Uint8 *pixels = (Uint8 *)surface->pixels;

  int bpp = surface->format->BytesPerPixel;

  int pitch = surface->pitch;



  for (int y = 0; y < img->height; ++y) 

  {

    for (int x = 0; x < img->width; ++x) 

    {

      Uint8 *p = pixels + y * pitch + x * bpp;

      Uint32 pixel_value;



      switch (bpp) 

      {

      case 1:

        pixel_value = *p;

        break;

      case 2:

        pixel_value = *(Uint16 *)p;

        break;

      case 3:

        if (SDL_BYTEORDER == SDL_BIG_ENDIAN)

          pixel_value = p[0] << 16 | p[1] << 8 | p[2];

        else

          pixel_value = p[0] | p[1] << 8 | p[2] << 16;

        break;

      case 4:

        pixel_value = *(Uint32 *)p;

        break;

      default:

        pixel_value = 0;

        break;

      }



      Uint8 r, g, b;

      SDL_GetRGB(pixel_value, surface->format, &r, &g, &b);



      img->pixels[y][x].r = r;

      img->pixels[y][x].g = g;

      img->pixels[y][x].b = b;

    }

  }



  SDL_UnlockSurface(surface);

}



/*

    Loads an image from its path and assign a label

*/

iImage *load_image(const char *image_path, int label)

{

  char *imcopy = strdup(image_path);

  SDL_Surface *surface = load_surface(imcopy);

  if (surface == NULL) 

  {

    return NULL;

  }



  iImage *img = create_image(surface->w, surface->h, imcopy);

  img->label = label;

  if (img == NULL) 

  {

    SDL_FreeSurface(surface);

    return NULL;

  }



  extract_pixels(surface, img);



  SDL_FreeSurface(surface);



  return img;

}



/*

    Saves an image at a given path

*/

void save_image(iImage *img, const char *image_path) 

{

  if (img == NULL || img->pixels == NULL) 

  {

    fprintf(stderr, "Error with pixels or image.\n");

    return;

  }



  SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(

      0, img->width, img->height, 24, SDL_PIXELFORMAT_RGB24);

  if (surface == NULL) 

  {

    fprintf(stderr, "Error with surface : %s\n", SDL_GetError());

    return;

  }



  SDL_LockSurface(surface);



  Uint8 *pixels = (Uint8 *)surface->pixels;

  int pitch = surface->pitch;



  for (int y = 0; y < img->height; y++) 

  {

    for (int x = 0; x < img->width; x++) 

    {

      pPixel current_pixel = img->pixels[y][x];

      Uint8 *p = pixels + y * pitch + x * 3;

      p[0] = current_pixel.r;

      p[1] = current_pixel.g;

      p[2] = current_pixel.b;

    }

  }



  SDL_UnlockSurface(surface);



  if (SDL_SaveBMP(surface, image_path) != 0) 

  {

    fprintf(stderr, "Error while saving the image : %s\n", SDL_GetError());

    SDL_FreeSurface(surface);

    return;

  }



  SDL_FreeSurface(surface);

}



void free_image(iImage *img) 

{

  if (img != NULL) {

    if (img->pixels != NULL) 

    {

      for (int i = 0; i < img->height; ++i) 

      {

        free(img->pixels[i]);

      }

      free(img->pixels);

    }

    if (img->path != NULL) 

    {

      free(img->path);

    }

    free(img);

  }

}



/*

    Crops a region from the original image and creates a new image

*/



iImage *create_subimage(const iImage *original, unsigned int x, unsigned int y,

                        unsigned int width, unsigned int height) 

{

  if (original == NULL) {

    err(EXIT_FAILURE, "original img is null");

  }



  if ((const int)(x + width) > original->width ||

      (const int)(y + height) > original->height) 

  {

    err(EXIT_FAILURE, "wrong coords");

  }



  iImage *subimg = create_image(width, height, original->path);

  if (subimg == NULL) 

  {

    err(EXIT_FAILURE, "creation failed");

  }



  for (unsigned int row = 0; row < height; ++row)

  {

    for (unsigned int col = 0; col < width; ++col) 

    {

      subimg->pixels[row][col] = original->pixels[y + row][x + col];

    }

  }



  subimg->label = original->label;



  return subimg;

}
