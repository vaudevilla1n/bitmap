#include "bitmap.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
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

#define FILE_HDR_SIZE				0x0e

#define FILE_HDR_MAGIC_WORD_OFFSET		0x00
#define FILE_HDR_FILE_SIZE_OFFSET		0x02
#define FILE_HDR_START_ADDR_OFFSET		0x0a

#define INFO_HDR_SIZE_OFFSET			0x0e
#define INFO_HDR_WIDTH_OFFSET			0x12
#define INFO_HDR_HEIGHT_OFFSET			0x14
#define INFO_HDR_BPP_OFFSET			0x18

typedef struct {
	uint8_t magic_word[2];
	uint32_t file_size;
	uint32_t start_addr;
} file_hdr_t;

typedef enum {
	UNKNOWN,
	BITMAPCOREHEADER,
	OS22XBITMAPHEADER,
	OS22XBITMAPHEADER_16,
	BITMAPINFOHEADER,
	BITMAPV4HEADER,
	BITMAPV5HEADER,
} info_hdr_type_t;

typedef struct {
	uint32_t size;
	info_hdr_type_t type;
	uint16_t width;
	uint16_t height;
	uint16_t bpp;
} info_hdr_t;

struct bitmap_t {
	const char *path;
	uint8_t *img;
	size_t img_size;

	bitmap_err_t err;

	file_hdr_t file_hdr;
	info_hdr_t info_hdr;
};

static const char *info_hdr_type_to_string(const info_hdr_type_t type) {
	switch (type) {
	case BITMAPCOREHEADER:		return "BITMAPCOREHEADER";
	case OS22XBITMAPHEADER_16:	return "OS22XBITMAPHEADER_16";
	case OS22XBITMAPHEADER:		return "OS22XBITMAPHEADER";
	case BITMAPINFOHEADER:		return "BITMAPINFOHEADER";
	case BITMAPV4HEADER:		return "BITMAPV4HEADER";
	case BITMAPV5HEADER:		return "BITMAPV5HEADER";

	default:			UNREACHABLE(info_hdr_type_to_string);
	}
}

static info_hdr_type_t determine_info_hdr_type(const uint32_t size) {
	switch (size) {
	case 12:	return BITMAPCOREHEADER;
	case 16:	return OS22XBITMAPHEADER_16;
	case 64:	return OS22XBITMAPHEADER;
	case 40:	return BITMAPINFOHEADER;
	case 108:	return BITMAPV4HEADER;
	case 124:	return BITMAPV5HEADER;

	default:	return UNKNOWN;
	}
}

static uint8_t *map_file(const char *file, const int oflag, const int prot, size_t *file_size) {
	struct stat fs = { 0 };
	if (stat(file, &fs))
		return NULL;

	const int fd = open(file, oflag);
	if (fd == -1)
		return NULL;

	void *m = mmap(NULL, fs.st_size, prot, MAP_PRIVATE, fd, 0);

	if (m == MAP_FAILED) {
		perror("mmamp");
		abort();
	}

	*file_size = fs.st_size;
	return m;
}

static void unmap_file(void *m, const size_t length) {
	if (munmap(m, length)) {
		perror("munmap");
		abort();
	}
}

static inline bool valid_magic_word(const uint8_t magic_word[2]) {
	return magic_word[0] == 'B' && magic_word[1] == 'M';
}

static inline void set_error(bitmap_err_t *ret, const bitmap_err_t err) {
	if (ret) *ret = err;
}

bitmap_t *bitmap_open(const char *file) {
	bitmap_t *bitmap = xcalloc(1, sizeof(*bitmap));
	bitmap->path = file;
	bitmap->img = map_file(file, O_RDONLY, PROT_READ, &bitmap->img_size);

	if (!bitmap->img) {
		bitmap->err = BITMAP_INVALID_PATH;
		return bitmap;
	}

	bitmap->file_hdr.magic_word[0] = bitmap->img[FILE_HDR_MAGIC_WORD_OFFSET];
	bitmap->file_hdr.magic_word[1] = bitmap->img[FILE_HDR_MAGIC_WORD_OFFSET + 1];

	if(!valid_magic_word(bitmap->file_hdr.magic_word))
		goto format_error;

	bitmap->file_hdr.file_size	= *(uint32_t *)(bitmap->img + FILE_HDR_FILE_SIZE_OFFSET);
	bitmap->file_hdr.start_addr	= *(uint32_t *)(bitmap->img + FILE_HDR_START_ADDR_OFFSET);

	bitmap->info_hdr.size = *(uint32_t *)(bitmap->img + INFO_HDR_SIZE_OFFSET);
	bitmap->info_hdr.type = determine_info_hdr_type(bitmap->info_hdr.size);

	if (bitmap->info_hdr.type == UNKNOWN)
		goto format_error;

	bitmap->info_hdr.width	= *(uint16_t *)(bitmap->img + INFO_HDR_WIDTH_OFFSET);
	bitmap->info_hdr.height	= *(uint16_t *)(bitmap->img + INFO_HDR_HEIGHT_OFFSET);
	bitmap->info_hdr.bpp	= *(uint16_t *)(bitmap->img + INFO_HDR_BPP_OFFSET);

	bitmap->err = BITMAP_OK;
	return bitmap;
format_error:
	bitmap->err = BITMAP_INVALID_FORMAT;
	return bitmap;
}

void bitmap_close(bitmap_t *b) {
	unmap_file(b->img, b->img_size);
	free(b);
}

bitmap_err_t bitmap_error(const bitmap_t *b) {
	return b->err;
}

static const char *err_to_string(const bitmap_err_t err) {
	switch (err) {
	case BITMAP_OK:			return "ok";
	case BITMAP_INVALID_PATH:	return "invalid path";
	case BITMAP_INVALID_FORMAT:	return "invalid format";
	
	default: UNREACHABLE(err_to_string);
	}
}

void bitmap_warn(const bitmap_t *b) {
	fprintf(stderr, "bitmap error: %s: %s\n", b->path, err_to_string(b->err));
}

void bitmap_info(const bitmap_t *b) {
	printf("\"%s\" (%uB)\n", b->path, b->file_hdr.file_size);
	printf("{ \'%c\' \'%c\'}\n", b->file_hdr.magic_word[0], b->file_hdr.magic_word[1]);
	printf("pixels @ 0x%x\n", b->file_hdr.start_addr);

	printf("\n%s (info header, %uB)\n", info_hdr_type_to_string(b->info_hdr.type),  b->info_hdr.size);
	printf("(%hu x %hu) %ubpp\n", b->info_hdr.width, b->info_hdr.height, b->info_hdr.bpp);
}
