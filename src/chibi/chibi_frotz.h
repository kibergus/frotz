#pragma once

#include <time.h>

extern int user_random_seed;

time_t time(time_t *tloc);
void* ch_malloc(size_t size);
void ch_free(void *ptr);
void* ch_realloc(void *ptr, size_t size);

void ch_fputs(const char* str);
void ch_fputc(char c);

// Implemented in frotz_controller.cpp
void frotz_getline(char *s, size_t n);
