#include "word_processor.h"
#include "letter_recognition.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>


# define MAX_N_LETTERS 20

int file_exists(const char *path) {
    FILE *f = fopen(path, "r");
    if (f) {
        fclose(f);
        return 1;
    }
    return 0;
}

int detect_words_number(const char *words_letters_dir)
{
    for(int i=0;i<100;i++)
    {
        char path[512];
        snprintf(path, sizeof(path), "%s/w_%02d.bmp",words_letters_dir, i);
        printf("%s",path);
        //test existence de la letter 
        if (!file_exists(path)) return i;
    }
    return 100;
}


// Placeholder for word recognition
int process_words(const char* words_dir,const char* words_letters_dir, const char* output_file) 
{
    
    printf("[WORDS] Words dir: %s\n", words_dir);
    printf("[WORDS] Output: %s\n", output_file);
    
    int number_words= detect_words_number(words_dir);
    printf("[WORDS] %d words found",number_words);
    
    // Allocate words
    char** words = (char**)malloc(number_words * sizeof(char*));
    for (int i = 0; i < number_words; i++) {
        words[i] = (char*)malloc((MAX_N_LETTERS + 1) * sizeof(char));
        words[i][MAX_N_LETTERS] = '\0';  // Null terminate each row
    }
    


    // Process each letter of each word
    printf("[GRID] Processing %d words...\n",number_words);

    for (int n_words=0; n_words<number_words; n_words++) {
        for (int n_letters = 0; n_letters < MAX_N_LETTERS; n_letters++) {
            char path[512];
            snprintf(path, sizeof(path), "%s/word_%02d_letter_%02d.bmp",words_letters_dir, n_words, n_letters);
            //test existence de la letter 
            if (file_exists(path))
            {
                char letter = recognize_letter(path);
                words[n_words][n_letters] = letter;
            }
            else
            {
                words[n_words][n_letters] ='\0'; // stop la reconnaisance du mot comme toutes les lettres ont été traitées
                break;
            }
        }
    }

    printf("[GRID] Recognition complete:\n");

    // Write to file
    FILE* f = fopen(output_file, "w");
    if (!f) {
        fprintf(stderr, "[GRID] ✗ Cannot create output file: %s\n", output_file);
        for (int i = 0; i < number_words; i++) free(words[i]);
        free(words);
        return -1;
    }

    for (int i = 0; i < number_words; i++) {
        fprintf(f, "%s\n", words[i]);
    }


    fclose(f);


    // Cleanup
    for (int i = 0; i < number_words; i++) free(words[i]);
    free(words);

    printf("[GRID] ✓ Words saved to: %s\n", output_file);


    
    return 0;
}
