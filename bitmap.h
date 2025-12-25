#pragma once

#include <stdint.h>

typedef struct bitmap_t bitmap_t;

typedef enum {
	BITMAP_OK,
	BITMAP_INVALID_PATH,
	BITMAP_INVALID_FORMAT,
} bitmap_err_t;

bitmap_t *bitmap_open(const char *file, bitmap_err_t *err);
void bitmap_close(bitmap_t *b);
void bitmap_error(const char *file, const bitmap_err_t err);
void bitmap_info(const bitmap_t *b);
