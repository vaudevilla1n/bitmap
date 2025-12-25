#include "bitmap.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static void *assert_ptr(void *p, const char *func) {
	if (!p) {
		perror(func);
		abort();
	}

	return p;
}

#define xcalloc(n, size)	assert_ptr(calloc((n), (size)), "calloc");

#define UNUSED(x)	(void)x
#define TODO(x)		do { fprintf(stderr, "todo: %s\n", #x); abort(); } while (0)
#define UNREACHABLE(x)	do { fprintf(stderr, "unreachable: %s\n", #x); abort(); } while (0)

#define BITMAP_HDR_SIZE		0x0e

#define OFFSET_MAGIC_WORD	0x00
#define OFFSET_FILE_SIZE	0x02
#define OFFSET_START_ADDR	0x0a

struct bitmap_t {
	const char *path;
	uint8_t magic_word[2];
	uint32_t file_size;
	uint32_t start_addr;
};

static inline bool valid_magic_word(const bitmap_t *b) {
	return b->magic_word[0] == 'B' && b->magic_word[1] == 'M';
}

static inline void set_error(bitmap_err_t *ret, const bitmap_err_t err) {
	if (ret) *ret = err;
}

bitmap_t *bitmap_open(const char *file, bitmap_err_t *err) {
	FILE *img = fopen(file, "r");
	if (!img) {
		set_error(err, BITMAP_INVALID_PATH);
		return NULL;
	}

	bitmap_t *bitmap = xcalloc(1, sizeof(*bitmap));

	bitmap->path = file;

	uint8_t hdr[BITMAP_HDR_SIZE];
	if (!fread(hdr, BITMAP_HDR_SIZE, 1, img))
		goto format_error;
	
	bitmap->magic_word[0] = hdr[OFFSET_MAGIC_WORD];
	bitmap->magic_word[1] = hdr[OFFSET_MAGIC_WORD + 1];

	if(!valid_magic_word(bitmap))
		goto format_error;
	
	bitmap->file_size = *(uint32_t *)(hdr + OFFSET_FILE_SIZE);
	bitmap->start_addr = *(uint32_t *)(hdr + OFFSET_START_ADDR);

	fclose(img);
	set_error(err, BITMAP_OK);
	return bitmap;

format_error:
	fclose(img);
	free(bitmap);
	set_error(err, BITMAP_INVALID_FORMAT);
	return NULL;;
}

void bitmap_close(bitmap_t *b) {
	free(b);
}

static const char *err_to_string(const bitmap_err_t err) {
	switch (err) {
	case BITMAP_OK:			return "ok";
	case BITMAP_INVALID_PATH:	return "invalid path";
	case BITMAP_INVALID_FORMAT:	return "invalid format";
	
	default: UNREACHABLE(err_to_string);
	}
}

void bitmap_error(const char *file, const bitmap_err_t err) {
	fprintf(stderr, "bitmap error: %s: %s\n", file, err_to_string(err));
}

void bitmap_info(const bitmap_t *b) {
	printf("\"%s\" (%uB)\n", b->path, b->file_size);
	printf("magic word ::= \'%c\' \'%c\'\n", b->magic_word[0], b->magic_word[1]);
	printf("pixel data start address ::= 0x%x\n", b->start_addr);
}
