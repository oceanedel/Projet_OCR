#include "letter_recognition.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <dirent.h>
#include <err.h>


/*
#define MAX_TEMPLATES_PER_LETTER 20
#define TEMPLATE_SIZE 32

typedef struct {
    SDL_Surface* templates[MAX_TEMPLATES_PER_LETTER];
    int count;
} LetterTemplates;

static LetterTemplates letter_templates[26];  // A-Z
static int total_templates = 0;
static double last_confidence = 0.0;

// Normalize image to fixed size
static SDL_Surface* normalize_size(SDL_Surface* img, int target_w, int target_h) {
    SDL_Surface* normalized = SDL_CreateRGBSurfaceWithFormat(
        0, target_w, target_h, 32, SDL_PIXELFORMAT_ARGB8888);
    
    if (!normalized) return NULL;
    
    SDL_Rect src = {0, 0, img->w, img->h};
    SDL_Rect dst = {0, 0, target_w, target_h};
    SDL_BlitScaled(img, &src, normalized, &dst);
    
    return normalized;
}

// Calculate similarity between two images
static double calculate_similarity(SDL_Surface* img1, SDL_Surface* img2) {
    if (!img1 || !img2) return 0.0;
    if (img1->w != img2->w || img1->h != img2->h) return 0.0;
    
    int W = img1->w, H = img1->h;
    
    if (SDL_MUSTLOCK(img1)) SDL_LockSurface(img1);
    if (SDL_MUSTLOCK(img2)) SDL_LockSurface(img2);
    
    uint8_t* pixels1 = (uint8_t*)img1->pixels;
    uint8_t* pixels2 = (uint8_t*)img2->pixels;
    int pitch1 = img1->pitch;
    int pitch2 = img2->pitch;
    
    int matching_pixels = 0;
    int total_pixels = W * H;
    
    for (int y = 0; y < H; y++) {
        uint32_t* row1 = (uint32_t*)(pixels1 + y * pitch1);
        uint32_t* row2 = (uint32_t*)(pixels2 + y * pitch2);
        
        for (int x = 0; x < W; x++) {
            int is_black1 = ((row1[x] & 0x00FFFFFF) == 0);
            int is_black2 = ((row2[x] & 0x00FFFFFF) == 0);
            
            if (is_black1 == is_black2) {
                matching_pixels++;
            }
        }
    }
    
    if (SDL_MUSTLOCK(img1)) SDL_UnlockSurface(img1);
    if (SDL_MUSTLOCK(img2)) SDL_UnlockSurface(img2);
    
    return (double)matching_pixels / total_pixels;
}

// Initialize with training templates
int letter_recognition_init(const char* training_dir) {
    printf("[OCR] Initializing letter recognition...\n");
    printf("[OCR] Loading templates from: %s\n", training_dir);
    
    // Initialize structures
    for (int i = 0; i < 26; i++) {
        letter_templates[i].count = 0;
        for (int j = 0; j < MAX_TEMPLATES_PER_LETTER; j++) {
            letter_templates[i].templates[j] = NULL;
        }
    }
    
    total_templates = 0;
    
    // Try multiple variants per letter
    for (int i = 0; i < 26; i++) {
        char letter = 'A' + i;
        
        // Try base template
        char path[512];
        snprintf(path, sizeof(path), "%s/%c.bmp", training_dir, letter);
        
        SDL_Surface* img = SDL_LoadBMP(path);
        if (img) {
            letter_templates[i].templates[0] = normalize_size(img, TEMPLATE_SIZE, TEMPLATE_SIZE);
            SDL_FreeSurface(img);
            letter_templates[i].count = 1;
            total_templates++;
            //printf("[OCR]   ✓ Loaded: %c.bmp\n", letter);
        }
        

        // Try additional variants
        for (int v = 1; v < MAX_TEMPLATES_PER_LETTER; v++) {
            snprintf(path, sizeof(path), "%s/%c_%d.bmp", training_dir, letter, v);
            
            img = SDL_LoadBMP(path);
            if (img) {
                letter_templates[i].templates[v] = normalize_size(img, TEMPLATE_SIZE, TEMPLATE_SIZE);
                SDL_FreeSurface(img);
                letter_templates[i].count++;
                total_templates++;
                //printf("[OCR]   ✓ Loaded: %c_%d.bmp\n", letter, v);
            }
        }
    }
    
    printf("[OCR] Loaded %d total templates across 26 letters\n", total_templates);
    
    
    if (total_templates == 0) {
        fprintf(stderr, "[OCR] ⚠️  No templates found!\n");
        return -1;
    }
    
    return 0;
}

// Recognize single letter
char recognize_letter(SDL_Surface* image) {
    if (!image || total_templates == 0) {
        last_confidence = 0.0;
        return '?';
    }
    
    // Normalize to template size
    SDL_Surface* normalized = normalize_size(image, TEMPLATE_SIZE, TEMPLATE_SIZE);
    if (!normalized) {
        last_confidence = 0.0;
        return '?';
    }
    
    // Compare against all templates for all letters
    double best_score = 0.0;
    char best_letter = '?';
    
    for (int i = 0; i < 26; i++) {
        char letter = 'A' + i;
        
        // Try all variants of this letter
        for (int v = 0; v < letter_templates[i].count; v++) {
            if (!letter_templates[i].templates[v]) continue;
            
            double score = calculate_similarity(normalized, letter_templates[i].templates[v]);
            
            if (score > best_score) {
                best_score = score;
                best_letter = letter;
            }
        }
    }
    
    SDL_FreeSurface(normalized);
    
    last_confidence = best_score;
    
    // Require at least 65% match
    if (best_score < 0.65) {
        return '?';
    }
    
    return best_letter;
}

// Get confidence of last recognition
double get_recognition_confidence() {
    return last_confidence;
}

// Cleanup
void letter_recognition_cleanup() {
    for (int i = 0; i < 26; i++) {
        for (int v = 0; v < letter_templates[i].count; v++) {
            if (letter_templates[i].templates[v]) {
                SDL_FreeSurface(letter_templates[i].templates[v]);
                letter_templates[i].templates[v] = NULL;
            }
        }
        letter_templates[i].count = 0;
    }
    total_templates = 0;
    printf("[OCR] Letter recognition cleanup complete\n");
}

*/






// OCR program

#define INPUT_SIZE 784 
#define HIDDEN_SIZE 32
#define OUTPUT_SIZE 26
#define LEARNING_RATE 0.1

float input[INPUT_SIZE];
float hidden[HIDDEN_SIZE];
float output[OUTPUT_SIZE];

float wIH[INPUT_SIZE][HIDDEN_SIZE]; // poids input->hidden
float bH[HIDDEN_SIZE];              // biais hidden

float wHO[HIDDEN_SIZE][OUTPUT_SIZE]; // poids hidden->output
float bO[OUTPUT_SIZE];               // biais output



//fonction sigmoid
float sigmoid(float x)
{
    return 1.0/(1.0+expf(-x));
}


//fonction softmax
void softmax(float *input, float *output, size_t len)
{
    float sum=0;
    for(size_t i=0;i<len; i++)
    {
        float e=expf(input[i]);
        sum+=e; //calcul sum
        output[i]=e; // copie de input dans output en appliquant exp
    }


    for (size_t i=0;i<len;i++)
    {
        output[i]/=sum;  //calcul de softmax
    }

}



//remplir un tableau 1D depuis un fichier texte
void load1D(const char *filename, float *array, int size) 
{
    FILE *fp = fopen(filename, "r");
    if (!fp) errx(EXIT_FAILURE, "Erreur : impossible d'ouvrir %s\n", filename);

    for (int i=0;i<size;i++) 
    {
        if (fscanf(fp,"%f",&array[i])!=1) 
        {
            errx(EXIT_FAILURE,"Erreur : lecture impossible dans %s à l'index %d\n",filename,i);
        }
    }
    fclose(fp);
}

//remplie une matrice 2D depuis un fichier texte
void load2D(const char *filename, int rows, int cols, float matrix[rows][cols])
{
    FILE *fp = fopen(filename, "r");
    if (!fp) errx(EXIT_FAILURE,"Erreur : impossible d'ouvrir %s\n",filename);

    for (int i=0;i<rows;i++) 
    {
        for (int j=0;j<cols;j++) 
        {
            if (fscanf(fp,"%f",&matrix[i][j])!=1) 
            {
                errx(EXIT_FAILURE,"Erreur : lecture impossible dans %s à [%d][%d]\n",filename,i,j);
            }
        }
    }
    fclose(fp);
}

//initialise les poids et biais à partir des fichiers
void init_weights() 
{
    load2D("wIH.txt",INPUT_SIZE, HIDDEN_SIZE,wIH);
    load2D("wHO.txt",HIDDEN_SIZE, OUTPUT_SIZE,wHO);
    load1D("bH.txt", bH, HIDDEN_SIZE);
    load1D("bO.txt", bO, OUTPUT_SIZE);
}




//calcul la couche cachée 
void calcul_hidden()
{
    for(int i=0;i<HIDDEN_SIZE;i++)
    {
        float tot=bH[i];
        for(int j=0;j<INPUT_SIZE;j++)
        {
            tot+=wIH[j][i]*input[j];
        }
        hidden[i]=sigmoid(tot);
    }
}


//calcul la couche de sortie
void calcul_output()
{
    float temp[OUTPUT_SIZE];
    for(int i=0;i<OUTPUT_SIZE;i++)
    {
        float tot=bO[i];
        for(int j=0;j<HIDDEN_SIZE;j++)
        {
            tot+=wHO[j][i]*hidden[j];
        }
        temp[i]=tot;
    }
    softmax(temp,output,OUTPUT_SIZE);

}


//lance le réseau de neurone et fait les calculs dans les différentes couches
void forward()
{
    calcul_hidden();
    calcul_output();
}



//renvoie le résultat de la reconnaissant de la lettre sur une image rentrée en paramètre sous la forme d'une matrice 
char letter_recognition(float **img, size_t len)
{
    if (len!=28) // 28 étant le nombre de pixel par ligne et colonne de la matrice image
    {
        errx(EXIT_FAILURE,"la matrice donnée ne possède pas la bonne taille");
    }
    init_weights();
    int n=0;
    for(size_t i=0;i<len;i++)
    {
        for(size_t j=0;j<len;j++)
        {
            input[n]=img[i][j];  //initialise l'input      
            n++;
        }
    }
    forward();

    int max=0;
    for(int i=0;i<OUTPUT_SIZE;i++)
    {
        printf("proba lettre %c : %.3f\n",'A'+i,output[i]);
        if (output[i]>output[max]) max=i;
    }

    printf("\nLetter reconnu : %c",'A'+max);
    return (char)('A'+max);
}


int main()
{
    return 0;
}
