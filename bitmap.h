#pragma once

#include <stdint.h>

typedef struct bitmap_t bitmap_t;

typedef enum {
	BITMAP_OK,
	BITMAP_INVALID_PATH,
	BITMAP_INVALID_FORMAT,
} bitmap_err_t;

typedef enum {
	BITMAP_RD, BITMAP_WR, BITMAP_RW,
} bitmap_access_t;

bitmap_t 	*bitmap_open(const char *file, const bitmap_access_t access);
void 		bitmap_close(bitmap_t *b);

bitmap_err_t 	bitmap_error(const bitmap_t *b);
void 		bitmap_warn(const bitmap_t *b);

void 		bitmap_info(const bitmap_t *b);
void		bitmap_display(const bitmap_t *b);
