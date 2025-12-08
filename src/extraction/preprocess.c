#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>


//DENOISER PART

static Uint8 median(Uint8 a[], int size) {
    // Simple bubble sort
    for (int i = 0; i < size - 1; i++) {
        for (int j = 0; j < size - i - 1; j++) {
            if (a[j] > a[j + 1]) {
                Uint8 tmp = a[j];
                a[j] = a[j + 1];
                a[j + 1] = tmp;
            }
        }
    }
    return a[size / 2];
}

static SDL_Surface* denoise_image(SDL_Surface* input) {
    if (!input) {
        fprintf(stderr, "[denoiser] Input surface is NULL\n");
        return NULL;
    }
    
    // Convert to 32-bit ARGB if needed
    SDL_Surface* input_32 = NULL;
    if (input->format->format != SDL_PIXELFORMAT_ARGB8888) {
        input_32 = SDL_ConvertSurfaceFormat(input, SDL_PIXELFORMAT_ARGB8888, 0);
        if (!input_32) {
            fprintf(stderr, "[denoiser] Format conversion failed: %s\n", SDL_GetError());
            return NULL;
        }
    } else {
        input_32 = input; // Already correct format
    }
    
    // Create output surface
    SDL_Surface* output = SDL_CreateRGBSurface(
        0,
        input_32->w, input_32->h,
        32,
        input_32->format->Rmask,
        input_32->format->Gmask,
        input_32->format->Bmask,
        input_32->format->Amask
    );
    
    if (!output) {
        fprintf(stderr, "[denoiser] Surface creation error: %s\n", SDL_GetError());
        if (input_32 != input) SDL_FreeSurface(input_32);
        return NULL;
    }
    
    SDL_LockSurface(input_32);
    SDL_LockSurface(output);
    
    int w = input_32->w;
    int h = input_32->h;
    
    // Copy border pixels as-is (no filtering)
    Uint32* in_pixels = (Uint32*)input_32->pixels;
    Uint32* out_pixels = (Uint32*)output->pixels;
    
    // Top and bottom rows
    for (int x = 0; x < w; x++) {
        out_pixels[x] = in_pixels[x]; // Top row
        out_pixels[(h - 1) * w + x] = in_pixels[(h - 1) * w + x]; // Bottom row
    }
    
    // Left and right columns
    for (int y = 0; y < h; y++) {
        Uint32* out_row = (Uint32*)((Uint8*)output->pixels + y * output->pitch);
        Uint32* in_row = (Uint32*)((Uint8*)input_32->pixels + y * input_32->pitch);
        out_row[0] = in_row[0]; // Left column
        out_row[w - 1] = in_row[w - 1]; // Right column
    }
    
    // Apply 3x3 median filter to interior pixels
    for (int y = 1; y < h - 1; y++) {
        for (int x = 1; x < w - 1; x++) {
            Uint8 r[9], g[9], b[9];
            int idx = 0;
            
            // Collect 3x3 neighborhood
            for (int dy = -1; dy <= 1; dy++) {
                Uint32* row = (Uint32*)((Uint8*)input_32->pixels + (y + dy) * input_32->pitch);
                
                for (int dx = -1; dx <= 1; dx++) {
                    Uint32 pixel = row[x + dx];
                    
                    Uint8 R, G, B;
                    SDL_GetRGB(pixel, input_32->format, &R, &G, &B);
                    
                    r[idx] = R;
                    g[idx] = G;
                    b[idx] = B;
                    idx++;
                }
            }
            
            // Apply median to each channel
            Uint32* out_row = (Uint32*)((Uint8*)output->pixels + y * output->pitch);
            out_row[x] = SDL_MapRGB(output->format,
                                    median(r, 9),
                                    median(g, 9),
                                    median(b, 9));
        }
    }
    
    SDL_UnlockSurface(input_32);
    SDL_UnlockSurface(output);
    
    // Free temporary surface if we created one
    if (input_32 != input) {
        SDL_FreeSurface(input_32);
    }
    
    printf("[denoiser] ✓ Median filter applied (%dx%d)\n", w, h);
    
    return output;
}






//PREPROCESS PART



static inline uint8_t gray_from_rgb(uint32_t px) {
    uint8_t r = (px >> 16) & 0xFF;
    uint8_t g = (px >> 8) & 0xFF;
    uint8_t b = px & 0xFF;
    return (uint8_t)((77 * r + 150 * g + 29 * b) >> 8);
}

static int is_image_noisy(SDL_Surface* surf, float* out_score)
{
    if (!surf) return 0;

    SDL_Surface* img = SDL_ConvertSurfaceFormat(surf, SDL_PIXELFORMAT_ARGB8888, 0);
    if (!img) return 0;

    int w = img->w, h = img->h;
    float total_var = 0.0f;
    int count = 0;

    SDL_LockSurface(img);

    for (int y = 1; y < h - 1; y++) {
        Uint32* row_m1 = (Uint32*)((Uint8*)img->pixels + (y - 1) * img->pitch);
        Uint32* row_0  = (Uint32*)((Uint8*)img->pixels +  y      * img->pitch);
        Uint32* row_p1 = (Uint32*)((Uint8*)img->pixels + (y + 1) * img->pitch);

        for (int x = 1; x < w - 1; x++) {
            // Collect 3×3 grayscale values
            float v[9];
            int k = 0;

            Uint32 pixels[3][3] = {
                { row_m1[x-1], row_m1[x], row_m1[x+1] },
                { row_0 [x-1], row_0 [x], row_0 [x+1] },
                { row_p1[x-1], row_p1[x], row_p1[x+1] }
            };

            for (int j = 0; j < 3; j++)
            for (int i = 0; i < 3; i++) {
                Uint8 r, g, b;
                SDL_GetRGB(pixels[j][i], img->format, &r, &g, &b);
                v[k++] = 0.299f*r + 0.587f*g + 0.114f*b; // grayscale
            }

            // compute variance
            float mean = 0;
            for (int i = 0; i < 9; i++) mean += v[i];
            mean /= 9.0f;

            float var = 0;
            for (int i = 0; i < 9; i++)
                var += (v[i] - mean) * (v[i] - mean);

            var /= 9.0f;

            total_var += var;
            count++;
        }
    }

    SDL_UnlockSurface(img);
    SDL_FreeSurface(img);

    float score = (count > 0 ? total_var / count : 0);

    if (out_score) *out_score = score;

    // Typical values:
    // Clean text image: ~2–5
    // Noisy/grainy:     ~20–80
    return score > 10.0f;
}

int isodata_threshold(const uint8_t *Y, int n) {
    long sum = 0;
    for (int i = 0; i < n; i++) sum += Y[i];

    int t = (int)(sum / n);
    int prev = -1;

    while (t != prev) {
        prev = t;
        long s0 = 0, s1 = 0;
        int n0 = 0, n1 = 0;

        for (int i = 0; i < n; i++) {
            if (Y[i] <= t) {
                s0 += Y[i];
                n0++;
            } else {
                s1 += Y[i];
                n1++;
            }
        }
        int m0 = n0 ? (int)(s0 / n0) : t;
        int m1 = n1 ? (int)(s1 / n1) : t;
        t = (m0 + m1) / 2;
    }
    return t;
}

SDL_Surface* binarize_image(const char* input_path) {
    SDL_Surface* src = SDL_LoadBMP(input_path);
    if (!src) {
        fprintf(stderr, "[preprocess] Failed to load BMP: %s\n", SDL_GetError());
        return NULL;
    }

    // Detect noise using 3x3 variance method
    float noise_score = 0.0f;
    int is_noisy = is_image_noisy(src, &noise_score);
    
    //DESACTIVATEUR !!
    is_noisy = 0;
    
    printf("[preprocess] Noise score: %.2f -> %s\n", 
           noise_score, is_noisy ? "NOISY" : "CLEAN");
    
    SDL_Surface* img = NULL;
    
    // Apply denoising if needed
    if (is_noisy) {
        printf("[preprocess] Applying median filter denoising...\n");
        
        SDL_Surface* denoised = denoise_image(src);
        SDL_FreeSurface(src);
        
        if (!denoised) {
            fprintf(stderr, "[preprocess] Denoising failed\n");
            return NULL;
        }
        
        // If very noisy (score > 30), apply second pass
        if (noise_score > 30.0f) {
            printf("[preprocess] Very noisy (%.2f), applying second pass...\n", noise_score);
            SDL_Surface* denoised2 = denoise_image(denoised);
            SDL_FreeSurface(denoised);
            denoised = denoised2;
        }
        
        img = SDL_ConvertSurfaceFormat(denoised, SDL_PIXELFORMAT_ARGB8888, 0);
        SDL_FreeSurface(denoised);
    } else {
        // Clean - no denoising
        printf("[preprocess] Clean image, skipping denoising\n");
        img = SDL_ConvertSurfaceFormat(src, SDL_PIXELFORMAT_ARGB8888, 0);
        SDL_FreeSurface(src);
    }
    
    if (!img) {
        fprintf(stderr, "[preprocess] Format conversion failed: %s\n", SDL_GetError());
        return NULL;
    }

    if (SDL_MUSTLOCK(img)) SDL_LockSurface(img);

    int W = img->w, H = img->h, N = W * H;
    uint32_t* p = (uint32_t*)img->pixels;

    uint8_t* Y = (uint8_t*)calloc(1, N);
    
    for (int i = 0; i < N; i++) {
        Y[i] = gray_from_rgb(p[i]);
    }
    
    int t = isodata_threshold(Y, N);
    printf("[preprocess] Threshold (ISODATA) = %d\n", t);

    SDL_Surface* bw = SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_ARGB8888);
    if (SDL_MUSTLOCK(bw)) SDL_LockSurface(bw);
    
    uint32_t* q = (uint32_t*)bw->pixels;
    for (int i = 0; i < N; i++) {
        uint8_t v = (Y[i] <= t) ? 0 : 255;
        q[i] = (0xFF << 24) | (v << 16) | (v << 8) | v;
    }

    if (SDL_MUSTLOCK(bw)) SDL_UnlockSurface(bw);
    if (SDL_MUSTLOCK(img)) SDL_UnlockSurface(img);

    free(Y);
    SDL_FreeSurface(img);
    return bw;
}
