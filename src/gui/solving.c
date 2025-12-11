#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/stat.h>
#include <SDL2/SDL.h>
#include "solving.h"

// Extraction modules
#include "../extraction/preprocess.h"
#include "../extraction/extract_grid.h"
#include "../extraction/slice_grid.h"
#include "../extraction/extract_wordlist.h"
#include "../extraction/slice_words.h"
#include "../extraction/slice_letter_word.h"
#include "../extraction/trim_cells.h"
#include "../extraction/trim_word_letters.h"
#include "../result/result.h"
#include "../extraction/slice_grid_no_lines.h"

// Solver wrapper
#include "../solver/solver.h"


// OCR modules
#include "../ocr/letter_recognition.h"
#include "../ocr/grid_processor.h"
#include "../ocr/word_processor.h"


#include <dirent.h>



// Clean output directory
void clean_output() {
    printf("[CLEANUP] Removing old output files...\n");
    int res;
    res=system("rm -f  ./output/binary.bmp ./output/grid.bmp ./output/solvingwords.bmp  2>/dev/null");
    res=system("rm -f ./output/grid.txt ./output/words.txt ./output/grid_before_autorotate.bmp  2>/dev/null");
    
    res=system("rm -f ./output/cells/*.bmp 2>/dev/null");
    res=system("rm -f ./output/words/*.bmp 2>/dev/null");
    res=system("rm -f ./output/word_letters/*.bmp 2>/dev/null");
    res=system("rm -f ./dataset/*/*.Identifier 2>/dev/null");
    (void)res;
    printf("[CLEANUP] ✓ Output directory cleaned\n");
}


int run_ocr_recognition(const char* cells_dir, const char* words_dir,const char* words_letters_dir, const char* output_file) {

    // Process grid
    int ret = process_grid(cells_dir, output_file);
    
    // Process words
    if (ret == 0) {
        process_words(words_dir,words_letters_dir,"./output/words.txt");
        
    }

    
    return ret;
}

static void print_highlighted_grid(int rows, int cols) {
    printf("\nSolved grid:\n\n");
    
    static const char *colors[] = {
        "\x1b[48;5;220m",  // Light orange
        "\x1b[48;5;156m",  // Light green  
        "\x1b[48;5;222m",  // Light yellow
        "\x1b[48;5;117m",  // Light blue
        "\x1b[48;5;201m",  // Light magenta
        "\x1b[48;5;123m",  // Light cyan
        "\x1b[48;5;215m",  // Light pink
        "\x1b[48;5;191m"   // Light lime
    };
    
    #define NUM_COLORS 8
    #define RESET "\x1b[0m"
    
    typedef struct {
        int x0, y0, x1, y1;
        int color_index;
    } FoundWord;
    
    FoundWord found_words[100];
    int found_count = 0;
    
    // Find all words and store coordinates
    FILE* words_file = fopen("./output/words.txt", "r");
    if (words_file) {
        char line[256];
        while (fgets(line, sizeof(line), words_file)) {
            line[strcspn(line, "\n\r")] = 0;
            if (strlen(line) == 0) continue;
            
            int x0, y0, x1, y1;
            if (find_word(line, &x0, &y0, &x1, &y1)) {
                found_words[found_count].x0 = x0;
                found_words[found_count].y0 = y0;
                found_words[found_count].x1 = x1;
                found_words[found_count].y1 = y1;
                found_words[found_count].color_index = found_count % NUM_COLORS;
                found_count++;
            }
        }
        fclose(words_file);
    }
    
    // Print grid with multi-color highlights
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            char c = grid[y][x];
            bool highlighted = false;
            
            // Check if this cell belongs to any word (last word wins for overlaps)
            for (int w = found_count - 1; w >= 0; w--) {
                int dx = (found_words[w].x1 > found_words[w].x0) ? 1 : 
                        ((found_words[w].x1 < found_words[w].x0) ? -1 : 0);
                int dy = (found_words[w].y1 > found_words[w].y0) ? 1 : 
                        ((found_words[w].y1 < found_words[w].y0) ? -1 : 0);
                int cx = found_words[w].x0, cy = found_words[w].y0;
                int steps = 0;
                int max_steps = abs(found_words[w].x1 - found_words[w].x0);
                if (abs(found_words[w].y1 - found_words[w].y0) > max_steps)
                    max_steps = abs(found_words[w].y1 - found_words[w].y0);
                
                while (steps <= max_steps) {
                    if (cx == x && cy == y) {
                        printf("%s%c%s ", colors[found_words[w].color_index], c, RESET);
                        highlighted = true;
                        break;
                    }
                    cx += dx;
                    cy += dy;
                    steps++;
                }
                if (highlighted) break;
            }
            
            if (!highlighted)
                printf("%c ", c);
        }
        printf("\n");
    }
    printf("\n");
}


int launch_solving(char *image_path, gpointer user_data) 
{
    AppData *app = (AppData*)user_data;

    gtk_label_set_text(GTK_LABEL(app->message_label), "Grid solving in progress...");
    gtk_widget_set_name(app->message_label, "message_error");
    if (image_path==NULL) return EXIT_FAILURE;
        
    clean_output();
    // Initialize SDL
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "✗ SDL Init Error: %s\n", SDL_GetError());
        return EXIT_FAILURE;
    }

    // Phase 1: Extraction
    printf("\n");
    printf("════════════════════════════════════════\n");
    printf("  Phase 1: Image Extraction\n");
    printf("════════════════════════════════════════\n");

    // Step 1: Binarize
    printf("\n[1/8] Binarizing image...\n");
    SDL_Surface* binary = binarize_image(image_path);
    if (!binary) {
        gtk_label_set_text(GTK_LABEL(app->message_label), "✗ Binarization failed");
        gtk_widget_set_name(app->message_label, "message_error"); 
        fprintf(stderr, "✗ Binarization failed\n");
        SDL_Quit();
        return EXIT_FAILURE;
    }

    mkdir("./output", 0755);
    SDL_SaveBMP(binary, "./output/binary.bmp");
    printf("  ✓ Binary image: output/binary.bmp\n");

    // Step 2: Extract grid
    printf("\n[2/8] Extracting puzzle grid...\n");
    int grid_x, grid_y, grid_w, grid_h;
    SDL_Surface* grid = extract_grid(binary, &grid_x, &grid_y, &grid_w, &grid_h);

    if (!grid) {
        gtk_label_set_text(GTK_LABEL(app->message_label), "✗ Grid extraction failed");
        gtk_widget_set_name(app->message_label, "message_error"); 
        fprintf(stderr, "✗ Grid extraction failed\n");
        SDL_FreeSurface(binary);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_SaveBMP(grid, "./output/grid.bmp");
    printf("  ✓ Grid region: (%d,%d) size %dx%d\n", grid_x, grid_y, grid_w, grid_h);
    printf("  ✓ Saved: output/grid.bmp\n");

    // Step 3: Slice grid into cells
    printf("\n[3/8] Slicing grid into cells...\n");
    mkdir("./output/cells", 0755);

    // Essayer d'abord la méthode "avec quadrillage"
    int slice_res = slice_grid(grid, "./output/cells");

    if (slice_res != 0) {
        printf("   ! Grid lines not found, trying 'No-Line' extraction method...\n");
        // Si ça échoue, essayer la méthode "sans quadrillage"
        if (slice_grid_no_lines(grid, "./output/cells") != 0) {
            gtk_label_set_text(GTK_LABEL(app->message_label), "✗ Grid slicing failed ");
            gtk_widget_set_name(app->message_label, "message_error");  
            fprintf(stderr, "✗ Grid slicing failed\n");
            SDL_FreeSurface(grid);
            SDL_FreeSurface(binary);
            SDL_Quit();
            return EXIT_FAILURE;
        }
    }

    SDL_FreeSurface(grid);
    printf("  ✓ Cell images: output/cells/\n");


    // Step 4: Trim cells
    printf("\n[4/8] Trimming cell whitespace...\n");
    if (trim_cells("./output/cells") != 0) {
        fprintf(stderr, "✗ Cell trimming failed\n");
        SDL_FreeSurface(binary);
        SDL_Quit();
        return EXIT_FAILURE;
    }
    printf("  ✓ Trimmed cells: output/cells/\n");

    // Step 5: Extract word list
    printf("\n[5/8] Extracting word list...\n");
    int wl_x, wl_y, wl_w, wl_h;
    SDL_Surface* wordlist = extract_wordlist(binary, grid_x, grid_y, grid_w, grid_h,
                                            &wl_x, &wl_y, &wl_w, &wl_h);
    SDL_FreeSurface(binary);

    if (!wordlist) {
        gtk_label_set_text(GTK_LABEL(app->message_label), "✗ Word list extraction failed");
        gtk_widget_set_name(app->message_label, "message_error");

        fprintf(stderr, "✗ Word list extraction failed\n");
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_SaveBMP(wordlist, "./output/solvingwords.bmp");
    printf("  ✓ Word list region: (%d,%d) size %dx%d\n", wl_x, wl_y, wl_w, wl_h);
    printf("  ✓ Saved: output/solvingwords.bmp\n");


    // Step 6: Slice word list
    printf("\n[6/8] Slicing word list...\n");
    mkdir("./output/words", 0755);

    if (slice_words(wordlist, "./output/words") != 0) {
        gtk_label_set_text(GTK_LABEL(app->message_label), "✗ Word slicing failed");
        gtk_widget_set_name(app->message_label, "message_error");
        fprintf(stderr, "✗ Word slicing failed\n");
        SDL_FreeSurface(wordlist);
        SDL_Quit();
        return EXIT_FAILURE;
    }

    SDL_FreeSurface(wordlist);
    printf("  ✓ Word images: output/words/\n");

    // Step 7: Slice word letters
    printf("\n[7/8] Slicing word letters...\n");
    mkdir("./output/word_letters", 0755);

    if (slice_word_letters("./output/words", "./output/word_letters") != 0) {
        gtk_label_set_text(GTK_LABEL(app->message_label), "✗ Word letter slicing failed");
        gtk_widget_set_name(app->message_label, "message_error");
        fprintf(stderr, "✗ Word letter slicing failed\n");
        SDL_Quit();
        return EXIT_FAILURE;
    }

    printf("  ✓ Letter images: output/word_letters/\n");

    // Step 8: Trim word letters
    printf("\n[8/8] Trimming word letter whitespace...\n");
    if (trim_word_letters("./output/word_letters") != 0) {
        gtk_label_set_text(GTK_LABEL(app->message_label), "✗ Word letter trimming failed");
        gtk_widget_set_name(app->message_label, "message_error");

        fprintf(stderr, "✗ Word letter trimming failed\n");
        SDL_Quit();
        return EXIT_FAILURE;
    }
    printf("  ✓ Trimmed letters: output/word_letters/\n");



    printf("\n✓ Extraction phase complete!\n");





    
    // Phase 2: OCR
    printf("\n");
    printf("========================================\n");
    printf("  Phase 2: OCR Recognition\n");
    printf("=========================================\n");
    
    if (run_ocr_recognition("./output/cells", "./output/words","./output/word_letters", "./output/grid.txt") != 0) {
        gtk_label_set_text(GTK_LABEL(app->message_label), "✗ OCR failed");
        gtk_widget_set_name(app->message_label, "message_error");
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
    read_grid("./output/grid.txt");
    printf("[SOLVER] Grid loaded: %d rows × %d cols\n", rows, cols);

    printf("[SOLVER] Loading words from: output/words.txt\n");

    FILE* words_file = fopen("./output/words.txt", "r");
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
            gtk_label_set_text(GTK_LABEL(app->message_label), "✗ No words found in file");
            gtk_widget_set_name(app->message_label, "message_error");
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
            gtk_label_set_text(GTK_LABEL(app->message_label), "SUCCESS! All words found!");
            gtk_widget_set_name(app->message_label, "message_error");
            show_result_window();
            printf("\n🎉 SUCCESS! All words found!\n");
        } else if (found_count > 0) {
            gtk_label_set_text(GTK_LABEL(app->message_label), "Partial success - not all the words found.");
            gtk_widget_set_name(app->message_label, "message_error");
            show_result_window();
            printf("\n⚠️  Partial success - %d/%d words found.\n", found_count, word_count);
        } else {
            gtk_label_set_text(GTK_LABEL(app->message_label), "No words found");
            gtk_widget_set_name(app->message_label, "message_error");
            printf("\n⚠️  No words found. Check OCR accuracy.\n");
        }
        
        // Cleanup
        for (int i = 0; i < word_count; i++) {
            free(words[i]);
        }
        free(words);
    }

    printf("\n");
    print_highlighted_grid(rows, cols);

    SDL_Quit();
    return EXIT_SUCCESS;
}


