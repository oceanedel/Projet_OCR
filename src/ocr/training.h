#ifndef TRAINING_H

#include <stddef.h>
#define TRAINING_H

float sigmoid(float x);
void softmax(float *input, float *output, size_t len);
float random_weight();
void init_weights();
void calcul_hidden();
void calcul_output();
void forward();
void calcul_errorO(float *errorO,int index_letter);
void calcul_errorH(float *errorH,float *errorO);
void update_HO(float *errorO);
void update_IH(float *errorH);
void back_propagation(int index_letter);
void training(int index_letter, float **img,int len);
int store_res();
#endif
