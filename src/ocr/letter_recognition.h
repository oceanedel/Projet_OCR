#ifndef LETTER_RECOGNITION_H
#define LETTER_RECOGNITION_H

#include <SDL2/SDL.h>

int letter_recognition_init(const char* training_dir);


char recognize_letter(SDL_Surface* image);

double get_recognition_confidence();

void letter_recognition_cleanup();

#endif
