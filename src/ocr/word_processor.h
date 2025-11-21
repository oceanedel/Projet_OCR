#ifndef WORD_PROCESSOR_H
#define WORD_PROCESSOR_H

int detect_words_number(const char *words_letters_dir);
int process_words(const char* words_dir,const char* words_letters_dir, const char* output_file);
#endif
