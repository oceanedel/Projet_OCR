#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <math.h>
#include <stddef.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <err.h>
#include <time.h>
#include <sys/stat.h>

#define INPUT_SIZE 784
#define HIDDEN_SIZE 32
#define OUTPUT_SIZE 26
#define LEARNING_RATE 0.01
#define MAX_TEMPLATES_PER_LETTER 1300
#define TEMPLATE_SIZE 32
#define EPOCHS 10  // Modifie cette valeur si tu veux plus/moins d'époques

float input[INPUT_SIZE];
float hidden[HIDDEN_SIZE];
float output[OUTPUT_SIZE];

float wIH[INPUT_SIZE][HIDDEN_SIZE]; // poids input-hidden
float bH[HIDDEN_SIZE];              // biais hidden

float wHO[HIDDEN_SIZE][OUTPUT_SIZE]; // poids hidden-output
float bO[OUTPUT_SIZE];               // biais output

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

void loads()
{
    load2D("../../output/wIH.txt",INPUT_SIZE, HIDDEN_SIZE,wIH);
    load2D("../../output/wHO.txt",HIDDEN_SIZE, OUTPUT_SIZE,wHO);
    load1D("../../output/bH.txt", bH, HIDDEN_SIZE);
    load1D("../../output/bO.txt", bO, OUTPUT_SIZE);
}

//fonction sigmoid
float sigmoid(float x)
{
    return 1.0f/(1.0f+expf(-x));
}

//fonction softmax
void softmax(float *input_arr, float *output_arr, size_t len)
{
    float maxv = input_arr[0];
    for (size_t i = 1; i < len; ++i) if (input_arr[i] > maxv) maxv = input_arr[i];

    float sum=0.0f;
    for(size_t i=0;i<len; i++)
    {
        float e=expf(input_arr[i] - maxv);
        sum+=e;
        output_arr[i]=e;
    }

    if (sum == 0.0f) sum = 1e-20f;
    for (size_t i=0;i<len;i++)
    {
        output_arr[i]/=sum;
    }
}

//calcul un poids random
float random_weight()
{
    float r = ((float)rand() / (float)RAND_MAX) - 0.5f;
    return r * 0.1f;
}

// initialise les poids et biais aléatoirement pour commencer
void init_weights()
{
    for(int i=0;i<INPUT_SIZE;i++)
    {
        for(int j=0;j<HIDDEN_SIZE;j++)
        {
            wIH[i][j]=random_weight();
        }
    }

    for(int i=0;i<HIDDEN_SIZE;i++)
    {
        for(int j=0;j<OUTPUT_SIZE;j++)
        {
            wHO[i][j]=random_weight();
        }
    }

    for(int i=0;i<HIDDEN_SIZE;i++)
    {
        bH[i]=random_weight();
    }

    for(int i=0;i<OUTPUT_SIZE;i++)
    {
        bO[i]=random_weight();
    }
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

//calcul erreur de la couche de sortie
void calcul_errorO(float *errorO,int index_letter)
{
    for(int i=0;i<OUTPUT_SIZE;i++)
    {
        errorO[i]=output[i]-(i==index_letter?1.0f:0.0f);
    }
}

//calcul erreur de la couche cachée
void calcul_errorH(float *errorH,float *errorO)
{
    for(int i=0;i<HIDDEN_SIZE;i++)
    {
        float new_value=0.0f;
        for (int j=0;j<OUTPUT_SIZE;j++)
        {
            new_value+=errorO[j]*wHO[i][j];
        }
        errorH[i]=new_value*hidden[i]*(1-hidden[i]);
    }
}

//met à jour les poids et biais vers la couche de sortie
void update_HO(float *errorO)
{
    for (int i=0;i<HIDDEN_SIZE;i++)
    {
        for (int j=0;j<OUTPUT_SIZE;j++)
        {
            wHO[i][j]-=LEARNING_RATE*errorO[j]*hidden[i] ;
        }
    }

    for(int i=0;i<OUTPUT_SIZE;i++)
    {
        bO[i]-=LEARNING_RATE*errorO[i];
    }
}

//met à jour les poids et biais vers la couche cachée
void update_IH(float *errorH)
{
    for (int i=0;i<INPUT_SIZE;i++)
    {
        for (int j=0;j<HIDDEN_SIZE;j++)
        {
            wIH[i][j]-=LEARNING_RATE*errorH[j]*input[i] ;
        }
    }

    for(int i=0;i<HIDDEN_SIZE;i++)
    {
        bH[i]-=LEARNING_RATE*errorH[i];
    }
}

// lance le recalcul des poids et biais en fonction du résultat voulu pour une letter
void back_propagation(int index_letter)
{
    float errorO[OUTPUT_SIZE];
    float errorH[HIDDEN_SIZE];
    calcul_errorO(errorO,index_letter);
    calcul_errorH(errorH,errorO);

    update_HO(errorO);
    update_IH(errorH);
}

//entraine le réseau avec des images données
void training(int index_letter, float **img)
{
    if (!img) errx(EXIT_FAILURE,"matrice nulle");
    for(int i=0;i<32;i++)  if(!img[i]) errx(EXIT_FAILURE,"ligne %d n'existe pas", i);

    int n=0;
    for(size_t i=0;i<TEMPLATE_SIZE && n < INPUT_SIZE;i++)
    {
        for(size_t j=0;j<TEMPLATE_SIZE && n < INPUT_SIZE;j++)
        {
            input[n]=img[i][j];
            n++;
        }
    }
    forward();
    back_propagation(index_letter);
}

//stocke le valeurs obtenues pour les biais et les poids dans des fichiers texte
int store_res()
{
    // écrit les poids input-hidden
    FILE *fp1 = fopen("../../output/wIH.txt", "w");
    if (!fp1)
    {
        printf("Erreur : impossible d'ouvrir le fichier wIH.txt\n");
        return -1;
    }
    for (int i=0;i<INPUT_SIZE ;i++)
    {
        for(int j=0;j<HIDDEN_SIZE;j++)
        {
            fprintf(fp1, "%.6f ", wIH[i][j]);
        }
        fprintf(fp1,"\n");
    }

    fclose(fp1);

    // écrit les poids hidden-output
    FILE *fp2 = fopen("../../output/wHO.txt", "w");
    if (!fp2)
    {
        printf("Erreur : impossible d'ouvrir le fichier wHO.txt\n");
        return -1;
    }
    for (int i=0;i<HIDDEN_SIZE ;i++)
    {
        for (int j=0;j<OUTPUT_SIZE;j++)
        {
            fprintf(fp2, "%.6f ", wHO[i][j]);
        }
        fprintf(fp2,"\n");
    }

    fclose(fp2);

    // écrit les biais input-hidden
    FILE *fp3 = fopen("../../output/bH.txt", "w");
    if (!fp3)
    {
        printf("Erreur : impossible d'ouvrir le fichier bH.txt\n");
        return -1;
    }
    for (int i=0;i<HIDDEN_SIZE ;i++)
    {
        fprintf(fp3, "%.6f ", bH[i]);
    }
    fprintf(fp3, "\n");
    fclose(fp3);

    // écrit les biais hidden-output
    FILE *fp4 = fopen("../../output/bO.txt", "w");
    if (!fp4)
    {
        printf("Erreur : impossible d'ouvrir le fichier bO.txt\n");
        return -1;
    }
    for (int i=0;i<OUTPUT_SIZE ;i++)
    {
        fprintf(fp4, "%.6f ", bO[i]);
    }
    fprintf(fp4, "\n");
    fclose(fp4);

    return 0;
}

typedef struct
{
    SDL_Surface* templates[MAX_TEMPLATES_PER_LETTER];
    int count;
} LetterTemplates;

static LetterTemplates letter_templates[26];  // A-Z
static int total_templates = 0;

// Normalize image to fixed size
SDL_Surface* normalize_size(SDL_Surface* img, int target_w, int target_h)
{
    if (!img) return NULL;

    SDL_Surface* normalized = SDL_CreateRGBSurfaceWithFormat(
        0, target_w, target_h, 32, SDL_PIXELFORMAT_ARGB8888);

    if (!normalized) return NULL;

    SDL_Rect src = {0, 0, img->w, img->h};
    SDL_Rect dst = {0, 0, target_w, target_h};
    if (SDL_BlitScaled(img, &src, normalized, &dst) != 0)
    {
        SDL_FreeSurface(normalized);
        return NULL;
    }

    return normalized;
}

// Helper : retourne 1 si 'name' se termine par une extension d'image connue (insensible à la casse)
static int is_image_filename(const char *name)
{
    size_t L = strlen(name);
    if (L < 4) return 0;
    const char *ext = name + L - 4;
    if (strcasecmp(ext, ".bmp") == 0) return 1;
    if (strcasecmp(ext, ".png") == 0) return 1;
    if (strcasecmp(ext, ".jpg") == 0) return 1;
    // handle .jpeg (5 chars)
    if (L >= 5 && strcasecmp(name + L - 5, ".jpeg") == 0) return 1;
    if (strcasecmp(ext, ".gif") == 0) return 1;
    return 0;
}

// Initialize with training templates
// Expects 'dossier' to contain subfolders named "A", "B", ..., "Z".
// Each subfolder is scanned and up to MAX_TEMPLATES_PER_LETTER image files are loaded.
void load_letter_template(const char *dossier)
{
    // Initialize structures
    for (int i = 0; i < 26; i++) {
        letter_templates[i].count = 0;
        for (int j = 0; j < MAX_TEMPLATES_PER_LETTER; j++) {
            letter_templates[i].templates[j] = NULL;
        }
    }

    total_templates = 0;

    for (int i = 0; i < 26; i++) {
        char letter = 'A' + i;
        char subdir[512];
        snprintf(subdir, sizeof(subdir), "%s/%c", dossier, letter);

        DIR *dir = opendir(subdir);
        if (!dir) {
            // Pas de dossier pour cette lettre : on continue sans erreur
            fprintf(stderr, "[OCR] Aucun dossier %s, on passe à la lettre suivante\n", subdir);
            continue;
        }

        struct dirent *entry;
        int loaded = 0;

        while ((entry = readdir(dir)) != NULL && loaded < MAX_TEMPLATES_PER_LETTER) {
            // Saute les . et ..
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

            // Vérifie l'extension image
            if (!is_image_filename(entry->d_name)) continue;

            char path[1024];
            snprintf(path, sizeof(path), "%s/%s", subdir, entry->d_name);

            SDL_Surface* img = IMG_Load(path); // supporte png/jpg/bmp via SDL_image
            if (!img) {
                fprintf(stderr, "[OCR] Impossible de charger %s : %s\n", path, IMG_GetError());
                continue;
            }

            SDL_Surface* norm = normalize_size(img, TEMPLATE_SIZE, TEMPLATE_SIZE);
            SDL_FreeSurface(img);

            if (!norm) {
                fprintf(stderr, "[OCR] normalize_size failed for %s: %s\n", path, SDL_GetError());
                continue;
            }

            letter_templates[i].templates[loaded] = norm;
            loaded++;
            total_templates++;
        }

        letter_templates[i].count = loaded;
        closedir(dir);
    }

    printf("[OCR] Loaded %d total templates across 26 letters\n", total_templates);

    if (total_templates == 0) {
        errx(EXIT_FAILURE, "[OCR] ⚠️  No templates found in any subdirectory of %s\n", dossier);
    }
}

// convertie une sdl_surface en une matrice de 32*32
float **surface_to_grayscale(SDL_Surface *img)
{
    if (!img) return NULL;

    // Convertit en format 32 bits ARGB8888 pour simplifier (cohérent avec normalize_size)
    SDL_Surface *converted = SDL_ConvertSurfaceFormat(img, SDL_PIXELFORMAT_ARGB8888, 0);
    if (!converted) {
        fprintf(stderr, "SDL_ConvertSurfaceFormat failed: %s\n", SDL_GetError());
        return NULL;
    }

    int w = converted->w;
    int h = converted->h;

    // Allocation du tableau 2D
    float **pixels = malloc(h * sizeof(float *));
    if (!pixels) {
        SDL_FreeSurface(converted);
        return NULL;
    }
    for (int y = 0; y < h; y++) {
        pixels[y] = malloc(w * sizeof(float));
        if (!pixels[y]) {
            for (int k=0;k<y;k++) free(pixels[k]);
            free(pixels);
            SDL_FreeSurface(converted);
            return NULL;
        }
    }

    Uint8 *p = (Uint8 *)converted->pixels;

    for (int y=0;y<h;y++) {
        for (int x=0;x<w;x++) {
            Uint32 pixel = *((Uint32 *)(p + y * converted->pitch + x * 4));
            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, converted->format, &r, &g, &b, &a);

            // Si l'image a de l'alpha (transparence), on peut traiter alpha==0 comme blanc (1.0)
            // Ici on ignore alpha et prend juste la moyenne RGB
            float gray = (r + g + b) / 3.0f / 255.0f;
            pixels[y][x] = gray;
        }
    }

    SDL_FreeSurface(converted);
    return pixels;
}

// lance l'entrainement avec un grand nombre d'exemple de lettres
int train()
{
    init_weights();
   // load_letter_template("../../dataset");
    load_letter_template("../../dataset");

    // Construire la liste des paires (lettre, variante)
    int available = 0;
    for (int i = 0; i < 26; i++) {
        for (int j = 0; j < MAX_TEMPLATES_PER_LETTER; j++) {
            if (letter_templates[i].templates[j]) available++;
        }
    }

    if (available == 0) {
        errx(EXIT_FAILURE, "Aucun template disponible pour l'entraînement");
    }

    typedef struct { int letter; int variant; } Item;
    Item *items = malloc(available * sizeof(Item));
    if (!items) errx(EXIT_FAILURE, "Allocation échouée");

    int idx = 0;
    for (int i = 0; i < 26; i++) {
        for (int j = 0; j < MAX_TEMPLATES_PER_LETTER; j++) {
            if (letter_templates[i].templates[j]) {
                items[idx].letter = i;
                items[idx].variant = j;
                idx++;
            }
        }
    }

    // Plusieurs époques (si EPOCHS>1) ; on shuffle à chaque époque (Fisher-Yates)
    for (int e = 0; e < EPOCHS; e++) {
        // Shuffle Fisher-Yates
        for (int k = available - 1; k > 0; k--) {
            int r = rand() % (k + 1);
            // swap items[k] et items[r]
            Item tmp = items[k];
            items[k] = items[r];
            items[r] = tmp;
        }

        // Entraînement sur l'ordre mélangé
        for (int t = 0; t < available; t++) {
            int li = items[t].letter;
            int vi = items[t].variant;
            SDL_Surface* img = letter_templates[li].templates[vi];
            if (!img) continue;
            float** img_pixel = surface_to_grayscale(img);
            if (!img_pixel) {
                fprintf(stderr, "surface_to_grayscale failed for letter %c template %d\n", 'A'+li, vi);
                continue;
            }
            training(li, img_pixel);
            for (int y = 0; y < TEMPLATE_SIZE; y++) free(img_pixel[y]);
            free(img_pixel);
        }
    }

    free(items);

    // free template surfaces (nous n'en avons plus besoin après training)
    for (int i=0;i<26;i++){
        for (int j=0;j<MAX_TEMPLATES_PER_LETTER;j++){
            if (letter_templates[i].templates[j]) {
                SDL_FreeSurface(letter_templates[i].templates[j]);
                letter_templates[i].templates[j] = NULL;
            }
        }
        letter_templates[i].count = 0;
    }

    int store=store_res();
    if (store!=0) errx(EXIT_FAILURE,"erreur ecriture fichier");
    return 0;
}

//renvoie le résultat de la reconnaissant de la lettre sur une image rentrée en paramètre sous la forme d'une matrice
char letter_recognition(float **img)
{
    loads();

    int n=0;
    for(size_t i=0;i<TEMPLATE_SIZE && n < INPUT_SIZE;i++)
    {
        for(size_t j=0;j<TEMPLATE_SIZE && n < INPUT_SIZE;j++)
        {
            input[n]=img[i][j];
            n++;
        }
    }
    forward();

    int max=0;
    for(int i=0;i<OUTPUT_SIZE;i++)
    {
       // printf("proba lettre %c : %.3f\n",'A'+i,output[i]);
        if (output[i]>output[max]) max=i;
    }

    printf("\nLetter reconnu : %c  avec une proba de %.3f\n",'A'+max,output[max]);
    return (char)('A'+max);
}


/*
int main(int argc, char **argv)
{
    if (argc!=2) errx(EXIT_FAILURE,"mauvais arguments. usage: %s image.png", argv[0]);

    // initialisation SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        errx(EXIT_FAILURE, "SDL_Init Failed: %s\n", SDL_GetError());
    }

    // initialisation SDL_image (PNG/JPG)
    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(img_flags) & img_flags) != img_flags) {
        fprintf(stderr, "IMG_Init Failed: %s\n", IMG_GetError());
        SDL_Quit();
        errx(EXIT_FAILURE, "Impossible d'initialiser SDL_image");
    }

    // initialisation aleatoire
    srand((unsigned int)time(NULL));

    // lance le training si tous les fichiers bH, bO ... n'existent pas
    FILE *file = fopen( "bH.txt", "r");
    if (file)
    {
        fclose(file);
    }
    else train();


    // Vérifie après training que les fichiers existent
    FILE *file1 = fopen( "bH.txt", "r");
    if (file1)
    {
        fclose(file1);
    }
    else {
        IMG_Quit();
        SDL_Quit();
        errx(EXIT_FAILURE,"erreur lors du training");
    }

    char *path=argv[1];

    SDL_Surface *img = IMG_Load(path); // utilise IMG_Load pour PNG/JPG/BMP
    if (!img) {
        IMG_Quit();
        SDL_Quit();
        errx(EXIT_FAILURE, "erreur lors du chargement de l'image: %s", IMG_GetError());
    }

    SDL_Surface *norm = normalize_size(img, TEMPLATE_SIZE, TEMPLATE_SIZE);
    SDL_FreeSurface(img);
    if (!norm) {
        IMG_Quit();
        SDL_Quit();
        errx(EXIT_FAILURE, "normalize_size a échoué");
    }

    float **img_pixel = surface_to_grayscale(norm);
    SDL_FreeSurface(norm);
    if (!img_pixel) {
        IMG_Quit();
        SDL_Quit();
        errx(EXIT_FAILURE, "surface_to_grayscale a échoué");
    }

    // Reconnaissance
    char res = letter_recognition(img_pixel);

    // Libération complète
    for (int y = 0; y < TEMPLATE_SIZE; y++) free(img_pixel[y]);
    free(img_pixel);

    printf("\nRésultat: %c\n", res);

    IMG_Quit();
    SDL_Quit();
    return 0;
}
*/

char recognize_letter(char *path_letter)
{
    // initialisation SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        errx(EXIT_FAILURE, "SDL_Init Failed: %s\n", SDL_GetError());
    }

    // initialisation SDL_image (PNG/JPG)
    int img_flags = IMG_INIT_PNG | IMG_INIT_JPG;
    if ((IMG_Init(img_flags) & img_flags) != img_flags) {
        fprintf(stderr, "IMG_Init Failed: %s\n", IMG_GetError());
        SDL_Quit();
        errx(EXIT_FAILURE, "Impossible d'initialiser SDL_image");
    }

    // initialisation aleatoire
    srand((unsigned int)time(NULL));

    // lance le training si tous les fichiers bH, bO ... n'existent pas
    FILE *file = fopen( "../../output/bH.txt", "r");
    
    if (file)
    {
        fclose(file);
    }
    else train();

    // Vérifie après training que les fichiers existent
    FILE *file1 = fopen( "../../output/bH.txt", "r");
    if (file1)
    {
        fclose(file1);
    }
    else {
        IMG_Quit();
        SDL_Quit();
        errx(EXIT_FAILURE,"erreur lors du training");
    }

    char *path=path_letter;

    SDL_Surface *img = IMG_Load(path); // utilise IMG_Load pour PNG/JPG/BMP
    if (!img) {
        IMG_Quit();
        SDL_Quit();
        errx(EXIT_FAILURE, "erreur lors du chargement de l'image: %s", IMG_GetError());
    }

    SDL_Surface *norm = normalize_size(img, TEMPLATE_SIZE, TEMPLATE_SIZE);
    SDL_FreeSurface(img);
    if (!norm) {
        IMG_Quit();
        SDL_Quit();
        errx(EXIT_FAILURE, "normalize_size a échoué");
    }

    float **img_pixel = surface_to_grayscale(norm);
    SDL_FreeSurface(norm);
    if (!img_pixel) {
        IMG_Quit();
        SDL_Quit();
        errx(EXIT_FAILURE, "surface_to_grayscale a échoué");
    }

    printf("image situé à l'endroit : %s",path);
    // Reconnaissance
    char res = letter_recognition(img_pixel);


    // Libération complète
    for (int y = 0; y < TEMPLATE_SIZE; y++) free(img_pixel[y]);
    free(img_pixel);

    IMG_Quit();
    SDL_Quit();
    return res;
}
