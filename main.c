#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <SDL2/SDL.h>

// Extraction modules
#include "src/extraction/preprocess.h"
#include "src/extraction/extract_grid.h"
#include "src/extraction/slice_grid.h"
#include "src/extraction/extract_wordlist.h"
#include "src/extraction/slice_words.h"
#include "src/extraction/slice_letter_word.h"
#include "src/extraction/trim_cells.h"


// Solver wrapper
#include "src/solver/solver.h"


// OCR modules
#include "src/ocr/letter_recognition.h"
#include "src/ocr/grid_processor.h"
#include "src/ocr/word_processor.h"


#include <dirent.h>

// Clean output directory
void clean_output() {
    printf("[CLEANUP] Removing old output files...\n");
    
    system("rm -f output/*.bmp output/*.txt 2>/dev/null");
    
    system("rm -f output/cells/*.bmp 2>/dev/null");
    system("rm -f output/words/*.bmp 2>/dev/null");
    system("rm -f output/word_letters/*.bmp 2>/dev/null");
    
    printf("[CLEANUP] ✓ Output directory cleaned\n");
}


int run_ocr_recognition(const char* cells_dir, const char* words_dir,const char* words_letters_dir, const char* output_file) {

    // Process grid
    int ret = process_grid(cells_dir, output_file);
    
    // Process words
    if (ret == 0) {
        process_words(words_dir,words_letters_dir,"output/words.txt");
        
    }
    
    // Cleanup OCR engine
//    letter_recognition_cleanup();
    
    return ret;
}



// Word list for testing
const char* test_words[] = {
    "IMAGINE", "RELAX", "COOL", "RESTING", "BREATHE", "EASY", "TENSION", "STRESS", "ALAAE"
};
const int test_word_count = 9;



int main(int argc, char** argv) {
    printf("\n");
    printf("╔════════════════════════════════════════╗\n");
    printf("║   Word Search Solver - Full Pipeline   ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("\n");
    
    clean_output();
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "✗ SDL Init Error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }
    
    // Get image path from user or argument
    char image_path[512];
    
    if (argc >= 2) {
        strncpy(image_path, argv[1], sizeof(image_path) - 1);
        image_path[sizeof(image_path) - 1] = '\0';
        printf("Input image: %s\n", image_path);
    }
    else
    {
        printf("Enter the path to your word search image: ");
        fflush(stdout);
        
        if (!fgets(image_path, sizeof(image_path), stdin)) {
            fprintf(stderr, "✗ Failed to read input\n");
            SDL_Quit();
            return EXIT_FAILURE;
        }
        
        image_path[strcspn(image_path, "\n")] = 0;
    }
    
    // Verify file exists
    if (access(image_path, F_OK) != 0) {
        fprintf(stderr, "✗ File not found: %s\n", image_path);
        SDL_Quit();
        return EXIT_FAILURE;
    }
    
    printf("✓ File found: %s\n", image_path);
    
    // Ask about rotation
    char rotate_response[10];
    printf("\nDo you want to rotate the image? (y/n): ");
    fflush(stdout);
    
    if (fgets(rotate_response, sizeof(rotate_response), stdin)) {
        rotate_response[strcspn(rotate_response, "\n")] = 0;
        
        if (rotate_response[0] == 'y' || rotate_response[0] == 'Y') {
            printf("\n[GUI] Launching image rotation tool...\n");
            printf("[GUI] Command: ./build/bin/image_rotator\n");
            printf("[GUI] Please rotate your image and save it, then press Enter to continue.\n\n");
            
            // Launch GUI rotation tool
            int ret = system("./build/bin/image_rotator &");
            if (ret != 0) {
                printf("[GUI] ⚠️  Could not launch rotation tool automatically.\n");
                printf("[GUI] You can manually run: ./build/bin/image_rotator\n");
            }
            
            printf("\nPress Enter when ready to continue with OCR...");
            getchar();
            
            // Ask for potentially updated image path
            printf("\nEnter image path (or press Enter to use original): ");
            char new_path[512];
            if (fgets(new_path, sizeof(new_path), stdin)) {
                new_path[strcspn(new_path, "\n")] = 0;
                if (strlen(new_path) > 0 && access(new_path, F_OK) == 0) {
                    strcpy(image_path, new_path);
                    printf("✓ Using rotated image: %s\n", image_path);
                }
            }
        }
    }
    



    // Phase 1: Extraction
    printf("\n");
    printf("════════════════════════════════════════\n");
    printf("  Phase 1: Image Extraction\n");
    printf("════════════════════════════════════════\n");

    // Step 1: Binarize
    printf("\n[1/7] Binarizing image...\n");
    SDL_Surface* binary = binarize_image(image_path);
    if (!binary) {
        fprintf(stderr, "✗ Binarization failed\n");
        SDL_Quit();
        return EXIT_FAILURE;
    }

    mkdir("output", 0755);
    SDL_SaveBMP(binary, "output/binary.bmp");
    printf("  ✓ Binary image: output/binary.bmp\n");

    // Step 2: Extract grid
    printf("\n[2/7] Extracting puzzle grid...\n");
    int grid_x, grid_y, grid_w, grid_h;
    SDL_Surface* grid = extract_grid(binary, &grid_x, &grid_y, &grid_w, &grid_h);

    if (!grid) {
        fprintf(stderr, "✗ Grid extraction failed\n");
        SDL_FreeSurface(binary);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_SaveBMP(grid, "output/grid.bmp");
    printf("  ✓ Grid region: (%d,%d) size %dx%d\n", grid_x, grid_y, grid_w, grid_h);
    printf("  ✓ Saved: output/grid.bmp\n");

    // Step 3: Slice grid into cells
    printf("\n[3/7] Slicing grid into cells...\n");
    mkdir("output/cells", 0755);

    if (slice_grid(grid, "output/cells") != 0) {
        fprintf(stderr, "✗ Grid slicing failed\n");
        SDL_FreeSurface(grid);
        SDL_FreeSurface(binary);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_FreeSurface(grid);
    printf("  ✓ Cell images: output/cells/\n");


    // Step 4: Trim cells
    if (trim_cells("output/cells") != 0) {
        fprintf(stderr, "✗ Cell trimming failed\n");
        SDL_FreeSurface(binary);
        SDL_Quit();
        return EXIT_FAILURE;
    }
    printf("  ✓ Trimmed cells: output/cells/\n");

    // Step 5: Extract word list
    printf("\n[5/7] Extracting word list...\n");
    int wl_x, wl_y, wl_w, wl_h;
    SDL_Surface* wordlist = extract_wordlist(binary, grid_x, grid_y, grid_w, grid_h,
                                            &wl_x, &wl_y, &wl_w, &wl_h);
    SDL_FreeSurface(binary);

    if (!wordlist) {
        fprintf(stderr, "✗ Word list extraction failed\n");
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_SaveBMP(wordlist, "output/solvingwords.bmp");
    printf("  ✓ Word list region: (%d,%d) size %dx%d\n", wl_x, wl_y, wl_w, wl_h);
    printf("  ✓ Saved: output/solvingwords.bmp\n");


    // Step 6: Slice word list
    printf("\n[6/7] Slicing word list...\n");
    mkdir("output/words", 0755);

    if (slice_words(wordlist, "output/words") != 0) {
        fprintf(stderr, "✗ Word slicing failed\n");
        SDL_FreeSurface(wordlist);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_FreeSurface(wordlist);
    printf("  ✓ Word images: output/words/\n");

    // Step 7: Slice word letters
    printf("\n[7/7] Slicing word letters...\n");
    mkdir("output/word_letters", 0755);

    if (slice_word_letters("output/words", "output/word_letters") != 0) {
        fprintf(stderr, "✗ Word letter slicing failed\n");
        SDL_Quit();
        return EXIT_FAILURE;
    }

    printf("  ✓ Letter images: output/word_letters/\n");

    /* Step 7: Trim word letters 
    printf("\n[7/7] Trimming word letter whitespace...\n");
    if (trim_cells("output/word_letters") != 0) {
        fprintf(stderr, "✗ Word letter trimming failed\n");
        SDL_Quit();
        return EXIT_FAILURE;
    }
    printf("  ✓ Trimmed letters: output/word_letters/\n");
*/


    printf("\n✓ Extraction phase complete!\n");





    
    // Phase 2: OCR
    printf("\n");
    printf("========================================\n");
    printf("  Phase 2: OCR Recognition\n");
    printf("=========================================\n");
    
    if (run_ocr_recognition("output/cells", "output/words","output/word_letters", "output/grid.txt") != 0) {
        fprintf(stderr, "✗ OCR failed\n");
        SDL_Quit();
        return EXIT_FAILURE;
    }
    
    printf("\n✓ OCR phase complete!\n");
    





    // Phase 3: Solve puzzle
    printf("\n");
    printf("════════════════════════════════════════\n");
    printf("  Phase 3: Solving Puzzle\n");
    printf("════════════════════════════════════════\n");

    printf("\n[SOLVER] Loading grid from: output/grid.txt\n");
    read_grid("output/grid.txt");
    printf("[SOLVER] Grid loaded: %d rows × %d cols\n", rows, cols);

    printf("[SOLVER] Loading words from: output/words.txt\n");

    FILE* words_file = fopen("output/words.txt", "r");
    if (!words_file) {
        fprintf(stderr, "[SOLVER] ⚠️  Warning: output/words.txt not found\n");        
    } 
    else {
        char** words = (char**)malloc(100 * sizeof(char*));
        int word_count = 0;
        char line[256];
        
        while (fgets(line, sizeof(line), words_file) && word_count < 100) {
            line[strcspn(line, "\n")] = 0;
            line[strcspn(line, "\r")] = 0;
            
            //skip empty lines
            if (strlen(line) == 0) continue;
            
            
            words[word_count] = strdup(line);
            word_count++;
        }
        
        fclose(words_file);
        
        printf("[SOLVER] Loaded %d words from file\n\n", word_count);
        
        if (word_count == 0) {
            fprintf(stderr, "[SOLVER] ✗ No words found in file\n");
            free(words);
            SDL_Quit();
            return EXIT_FAILURE;
        }
        
        printf("Searching for words...\n");
        printf("─────────────────────────────────────────\n");
        
        int found_count = 0;
        for (int i = 0; i < word_count; i++) {
            int x0, y0, x1, y1;
            printf("%-15s : ", words[i]);
            fflush(stdout);
            
            if (find_word(words[i], &x0, &y0, &x1, &y1)) {
                printf("✓ Found at (%d,%d) → (%d,%d)\n", x0, y0, x1, y1);
                found_count++;
            } else {
                printf("✗ Not found\n");
            }
        }
        
        printf("─────────────────────────────────────────\n");
        printf("Result: %d/%d words found\n", found_count, word_count);
        
        if (found_count == word_count) {
            printf("\n🎉 SUCCESS! All words found!\n");
        } else if (found_count > 0) {
            printf("\n⚠️  Partial success - %d/%d words found.\n", found_count, word_count);
        } else {
            printf("\n⚠️  No words found. Check OCR accuracy.\n");
        }
        
        // Cleanup
        for (int i = 0; i < word_count; i++) {
            free(words[i]);
        }
        free(words);
    }

    printf("\n");

    SDL_Quit();
    return EXIT_SUCCESS;
}


