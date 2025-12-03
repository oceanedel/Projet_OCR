#ifndef ROTATION_H
#define ROTATION_H

#include "image.h"

#define ROTATION_THRESHOLD 5.0
void rotation_matrix(double theta_deg, double R[2][2]);

void rotate_point(double x, double y, double center_x, double center_y,
                  double R[2][2], double *new_x, double *new_y);

void compute_new_dimensions(unsigned int width, unsigned int height,
                            double angle_deg, unsigned int *new_width,
                            unsigned int *new_height);


iImage *rotate_image(iImage *image, double angle_deg);

void detect_edges(iImage *image, double **edges, double **angles);

double find_dominant_angle(double **edges, double **angles, unsigned int width,
                           unsigned int height);

double determine_rotation_angle(iImage *image);

char *rotate_image_auto(char *path);

char *rotate_original_auto(char *path);

#endif // ROTATION_H
