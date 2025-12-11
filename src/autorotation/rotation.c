#include "rotation.h"
#include "image.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
/*
    Compute the 2D rotation matrix for a given angle in degrees.
*/
void rotation_matrix(double theta_deg, double R[2][2])
{
  double theta = theta_deg * PI / 180.0;
  R[0][0] = cos(theta);
  R[0][1] = -sin(theta);
  R[1][0] = sin(theta);
  R[1][1] = cos(theta);
}

/*
    Rotate a point (x, y) around a center using a rotation matrix.
*/
void rotate_point(double x, double y, double center_x, double center_y,
                  double R[2][2], double *new_x, double *new_y)
{
  double x_centered = x - center_x;
  double y_centered = y - center_y;

  *new_x = R[0][0] * x_centered + R[0][1] * y_centered + center_x;
  *new_y = R[1][0] * x_centered + R[1][1] * y_centered + center_y;
}

/*
  Its name talks for itself
*/
void compute_new_dimensions(unsigned int width, unsigned int height,
                            double angle_deg, unsigned int *new_width,
                            unsigned int *new_height)
{
  double angle_rad = angle_deg * PI / 180.0;

  double cos_theta = fabs(cos(angle_rad));
  double sin_theta = fabs(sin(angle_rad));

  *new_width = (unsigned int)(width * cos_theta + height * sin_theta);
  *new_height = (unsigned int)(width * sin_theta + height * cos_theta);
}

/*
  Its name talks for itself
*/
iImage *rotate_image(iImage *image, double angle_deg)
{

  unsigned int width = image->width;
  unsigned int height = image->height;

  unsigned int new_width, new_height;
  compute_new_dimensions(width, height, angle_deg, &new_width, &new_height);

  double center_x = width / 2.0;
  double center_y = height / 2.0;
  double new_center_x = new_width / 2.0;
  double new_center_y = new_height / 2.0;

  double R[2][2];
  rotation_matrix(-angle_deg, R);

  iImage *rotated_image = (iImage *)malloc(sizeof(iImage));
  rotated_image->width = new_width;
  rotated_image->height = new_height;
  rotated_image->pixels = (pPixel **)malloc(new_height * sizeof(pPixel *));
  for (unsigned int i = 0; i < new_height; i++)
  {
    rotated_image->pixels[i] = (pPixel *)malloc(new_width * sizeof(pPixel));
    for (unsigned int j = 0; j < new_width; j++)
    {
      rotated_image->pixels[i][j].r = 255;
      rotated_image->pixels[i][j].g = 255;
      rotated_image->pixels[i][j].b = 255;
    }
  }

  for (unsigned int y = 0; y < new_height; y++)
  {
    for (unsigned int x = 0; x < new_width; x++)
    {
      double src_x, src_y;

      rotate_point(x, y, new_center_x, new_center_y, R, &src_x, &src_y);

      src_x -= (new_center_x - center_x);
      src_y -= (new_center_y - center_y);

      if (src_x >= 0 && src_x < width - 1 && src_y >= 0 && src_y < height - 1)
      {
        int x0 = (int)floor(src_x);
        int y0 = (int)floor(src_y);
        int x1 = x0 + 1;
        int y1 = y0 + 1;

        double dx = src_x - x0;
        double dy = src_y - y0;

        pPixel p00 = image->pixels[y0][x0];
        pPixel p01 = image->pixels[y0][x1];
        pPixel p10 = image->pixels[y1][x0];
        pPixel p11 = image->pixels[y1][x1];

        rotated_image->pixels[y][x].r = (1 - dx) * (1 - dy) * p00.r +
                                        dx * (1 - dy) * p01.r +
                                        (1 - dx) * dy * p10.r + dx * dy * p11.r;

        rotated_image->pixels[y][x].g = (1 - dx) * (1 - dy) * p00.g +
                                        dx * (1 - dy) * p01.g +
                                        (1 - dx) * dy * p10.g + dx * dy * p11.g;

        rotated_image->pixels[y][x].b = (1 - dx) * (1 - dy) * p00.b +
                                        dx * (1 - dy) * p01.b +
                                        (1 - dx) * dy * p10.b + dx * dy * p11.b;
      }
    }
  }

  return rotated_image;
}

/*
  Its name talks for itself
*/
void detect_edges(iImage *image, double **edges, double **angles)
{
  unsigned int width = image->width;
  unsigned int height = image->height;

  for (unsigned int y = 1; y < height - 1; y++)
  {
    for (unsigned int x = 1; x < width - 1; x++)
    {
      double gx =
          image->pixels[y - 1][x + 1].r - image->pixels[y - 1][x - 1].r +
          2 * image->pixels[y][x + 1].r - 2 * image->pixels[y][x - 1].r +
          image->pixels[y + 1][x + 1].r - image->pixels[y + 1][x - 1].r;

      double gy =
          image->pixels[y - 1][x - 1].r + 2 * image->pixels[y - 1][x].r +
          image->pixels[y - 1][x + 1].r - image->pixels[y + 1][x - 1].r -
          2 * image->pixels[y + 1][x].r - image->pixels[y + 1][x + 1].r;

      double magnitude = sqrt(gx * gx + gy * gy);

      if (magnitude > 50.0)
      {
        edges[y][x] = magnitude;

        double gradient_angle = atan2(gy, gx) * 180.0 / PI;

        double angle = gradient_angle;
        
        // 1. On ramène tout entre -90 et 90
        while (angle <= -90.0) angle += 180.0;
        while (angle > 90.0)   angle -= 180.0;

        if (angle > 45.0) angle -= 90.0;
        else if (angle <= -45.0) angle += 90.0;

        angles[y][x] = angle;

      } else {
        edges[y][x] = 0.0;
        angles[y][x] = 0.0;
      }
    }
  }
}

/*
  Its name talks for itself
*/
double find_dominant_angle(double **edges, double **angles, unsigned int width, unsigned int height)
{
  int angle_bins[180] = {0};

  for (unsigned int y = 1; y < height - 1; y++)
  {
    for (unsigned int x = 1; x < width - 1; x++)
    {
      if (edges[y][x] > 0)
      {
        double angle = angles[y][x];

        while (angle < 0.0)
{
          angle += 180.0;
        }
        while (angle >= 180.0)
{
          angle -= 180.0;
        }

        int bin = (int)(angle + 0.5);
        if (bin >= 0 && bin < 180)
{
          angle_bins[bin]++;
        }
      }
    }
  }

  int max_votes = 0;
  int dominant_angle = 0;
  for (int i = 0; i < 180; i++)
  {
    if (angle_bins[i] > max_votes)
    {
      max_votes = angle_bins[i];
      dominant_angle = i;
    }
  }

  return (double)dominant_angle;
}

/*
  Its name talks for itself
*/
double determine_rotation_angle(iImage *image)
{
  unsigned int width = image->width;
  unsigned int height = image->height;

  double **edges = (double **)malloc(height * sizeof(double *));
  double **angles = (double **)malloc(height * sizeof(double *));
  for (unsigned int i = 0; i < height; i++) {
    edges[i] = (double *)calloc(width, sizeof(double));
    angles[i] = (double *)calloc(width, sizeof(double));
  }

  detect_edges(image, edges, angles);

  double dominant_angle = find_dominant_angle(edges, angles, width, height);

  for (unsigned int i = 0; i < height; i++)
  {
    free(edges[i]);
    free(angles[i]);
  }
  free(edges);
  free(angles);

  if (dominant_angle > 90.0)
  {
    dominant_angle -= 180.0;
  }

  while (dominant_angle > 45.0) {
      dominant_angle -= 90.0;
  }
  while (dominant_angle <= -45.0) {
      dominant_angle += 90.0;
  }

  if (fabs(dominant_angle) < ROTATION_THRESHOLD)
  {
    return 0.0;
  }

  return dominant_angle+4.0;
}

/*
  Its name talks for itself
*/
char *rotate_image_auto(char *path)
{
  iImage *image = load_image(path, -1);

  if (!image) {
    return NULL;
  }

  double angle_deg = determine_rotation_angle(image);

  iImage *res = rotate_image(image, angle_deg);

 // binary(res);
  //apply_groups_reduction(res);

  save_image(res, "resources/cache/pretraited.png");

  free_image(image);
  free_image(res);

  return strdup("resources/cache/pretraited.png");
}

char *rotate_original_auto(char *path) {
  iImage *image = load_image(path, -1);

  if (!image) {
    return NULL;
  }

  double angle_deg = determine_rotation_angle(image);

  iImage *res = rotate_image(image, angle_deg);

  save_image(res, "./output/image.bmp");

  free_image(image);
  free_image(res);

  return strdup("./output/image.bmp");
}
