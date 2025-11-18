#ifndef TRAINING_H
#define TRAINING_H

#include <stdlib.h>
#include <stdint.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#ifndef INPUT_SIZE
#define INPUT_SIZE 784
#endif

#ifndef HIDDEN_SIZE
#define HIDDEN_SIZE 32
#endif

#ifndef OUTPUT_SIZE
#define OUTPUT_SIZE 26
#endif

#ifndef LEARNING_RATE
#define LEARNING_RATE 0.1f
#endif

#ifndef MAX_TEMPLATES_PER_LETTER
#define MAX_TEMPLATES_PER_LETTER 20
#endif

#ifndef TEMPLATE_SIZE
#define TEMPLATE_SIZE 32
#endif

#ifndef EPOCHS
#define EPOCHS 5
#endif

extern float input[INPUT_SIZE];
extern float hidden[HIDDEN_SIZE];
extern float output[OUTPUT_SIZE];

extern float wIH[INPUT_SIZE][HIDDEN_SIZE];
extern float bH[HIDDEN_SIZE];

extern float wHO[HIDDEN_SIZE][OUTPUT_SIZE];
extern float bO[OUTPUT_SIZE];

typedef struct {
    SDL_Surface *templates[MAX_TEMPLATES_PER_LETTER];
    int count;
} LetterTemplates;

extern LetterTemplates letter_templates[26];
extern int total_templates;

void load1D(const char *filename, float *array, int size);
void load2D(const char *filename, int rows, int cols, float matrix[rows][cols]);
void loads(void);
int store_res(void);

float random_weight(void);
void init_weights(void);

float sigmoid(float x);
void softmax(float *input, float *output, size_t len);

void calcul_hidden(void);
void calcul_output(void);
void forward(void);

void calcul_errorO(float *errorO, int index_letter);
void calcul_errorH(float *errorH, float *errorO);
void update_HO(float *errorO);
void update_IH(float *errorH);
void back_propagation(int index_letter);

void training(int index_letter, float **img);

void load_letter_template(const char *dossier);

SDL_Surface *normalize_size(SDL_Surface *img, int target_w, int target_h);
float **surface_to_grayscale(SDL_Surface *img);

int train(void);
char letter_recognition(float **img);

int is_image_filename(const char *name);

#endif /* TRAINING_H */
