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

#define FILE_HDR_SIZE					0x0e

#define FILE_HDR_MAGIC_WORD_OFFSET			0x00
#define FILE_HDR_FILE_SIZE_OFFSET			0x02
#define FILE_HDR_START_ADDR_OFFSET			0x0a

#define INFO_HDR_SIZE_OFFSET				0x0e

#define INFO_HDR_OS2_WIDTH_OFFSET			0x12
#define INFO_HDR_OS2_HEIGHT_OFFSET			0x14
#define INFO_HDR_OS2_BPP_OFFSET				0x18

#define INFO_HDR_WIN_WIDTH_OFFSET			0x12
#define INFO_HDR_WIN_HEIGHT_OFFSET			0x16
#define INFO_HDR_WIN_BPP_OFFSET				0x1c

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

typedef struct {
	uint32_t size;
	info_hdr_type_t type;
	uint32_t width;
	uint32_t height;
	uint16_t bpp;
} info_hdr_t;

typedef enum {
	BOTTOM_UP,
	TOP_DOWN,
} pixel_order_t;

struct bitmap_t {
	const char *path;
	uint8_t *img;
	size_t img_size;

	bitmap_err_t err;

	file_hdr_t file_hdr;
	info_hdr_t info_hdr;

	size_t row_size;
	pixel_order_t pixel_order;
};


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

static uint8_t *memory_map_file(const char *file, const bitmap_access_t access, size_t *file_size) {
	struct stat fs = { 0 };
	if (stat(file, &fs))
		return NULL;

	int oflag;
	int prot;
	switch (access) {
	case BITMAP_RD: oflag = O_RDONLY; prot = PROT_READ; break;
	case BITMAP_WR:	oflag = O_WRONLY; prot = PROT_WRITE; break;
	case BITMAP_RW:	oflag = O_RDWR; prot = PROT_READ | PROT_WRITE; break;

	default: UNREACHABLE(memory_map_file);
	}

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

static void memory_unmap_file(void *m, const size_t length) {
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

static size_t calculate_row_size(const size_t width, const size_t bpp) {
	return ((width * bpp + 31) / 32) * 4;
}

bitmap_t *bitmap_open(const char *file, const bitmap_access_t access) {
	bitmap_t *bitmap = xcalloc(1, sizeof(*bitmap));
	bitmap->path = file;

	bitmap->img = memory_map_file(file, access, &bitmap->img_size);

	if (!bitmap->img) {
		bitmap->err = BITMAP_INVALID_PATH;
		return bitmap;
	}

	bitmap->file_hdr.magic_word[0] = bitmap->img[FILE_HDR_MAGIC_WORD_OFFSET];
	bitmap->file_hdr.magic_word[1] = bitmap->img[FILE_HDR_MAGIC_WORD_OFFSET + 1];

	if(!valid_magic_word(bitmap->file_hdr.magic_word))
		goto format_error;

	bitmap->file_hdr.file_size	= *(uint32_t *)(bitmap->img + FILE_HDR_FILE_SIZE_OFFSET);

	if (bitmap->file_hdr.file_size != bitmap->img_size)
		goto format_error;

	bitmap->file_hdr.start_addr	= *(uint32_t *)(bitmap->img + FILE_HDR_START_ADDR_OFFSET);

	bitmap->info_hdr.size = *(uint32_t *)(bitmap->img + INFO_HDR_SIZE_OFFSET);
	bitmap->info_hdr.type = determine_info_hdr_type(bitmap->info_hdr.size);

	switch (bitmap->info_hdr.type) {
	case BITMAPCOREHEADER:	
	case OS22XBITMAPHEADER_16:
	case OS22XBITMAPHEADER:	{
		bitmap->info_hdr.width	= *(uint16_t *)(bitmap->img + INFO_HDR_OS2_WIDTH_OFFSET);
		bitmap->info_hdr.height	= *(uint16_t *)(bitmap->img + INFO_HDR_OS2_HEIGHT_OFFSET);
		bitmap->info_hdr.bpp	= *(uint16_t *)(bitmap->img + INFO_HDR_OS2_BPP_OFFSET);
	} break;

	case BITMAPINFOHEADER:
	case BITMAPV4HEADER:
	case BITMAPV5HEADER: {
		const int32_t w = *(int32_t *)(bitmap->img + INFO_HDR_WIN_WIDTH_OFFSET);
		const int32_t h = *(int32_t *)(bitmap->img + INFO_HDR_WIN_HEIGHT_OFFSET);

		if (h < 0) {
			bitmap->pixel_order = TOP_DOWN;
			bitmap->info_hdr.height	= -h;
		} else {
			bitmap->info_hdr.height	= h;
		}

		bitmap->info_hdr.width = w; 
		bitmap->info_hdr.bpp = *(uint16_t *)(bitmap->img + INFO_HDR_WIN_BPP_OFFSET);
	} break;
	
	default: goto format_error;
	}

	bitmap->row_size = calculate_row_size(bitmap->info_hdr.width, bitmap->info_hdr.bpp);

	bitmap->err = BITMAP_OK;
	return bitmap;
format_error:
	bitmap->err = BITMAP_INVALID_FORMAT;
	return bitmap;
}

void bitmap_close(bitmap_t *b) {
	memory_unmap_file(b->img, b->img_size);
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
	printf("\"%s\" (%uB) ", b->path, b->file_hdr.file_size);
	printf("\'%c\' \'%c\' ", b->file_hdr.magic_word[0], b->file_hdr.magic_word[1]);
	printf("%ux%u (%zuB row) %ubpp ", b->info_hdr.width, b->info_hdr.height, b->row_size, b->info_hdr.bpp);
	printf("%s (%uB)\n", info_hdr_type_to_string(b->info_hdr.type),  b->info_hdr.size);
}

typedef struct {
	uint8_t r, g, b;
} rgb_t;

void rgb_print(const rgb_t c) {
	printf("\033[38;2;%u;%u;%um\033[48;2;%u;%u;%um|||||\033[0m", c.r, c.g, c.b, c.r, c.g, c.b);
	//printf("\033[38;2;%u;%u;%um\033[48;2;%u;%u;%umthese things take time\n\033[0m", c.r, c.g, c.b, c.r, c.g, c.b);
	//printf("\033[38;2;%u;%u;%umthese things take time\n\033[0m", c.r, c.g, c.b);
}

static rgb_t pixel_to_rgb(void *p, const size_t bpp) {
	switch (bpp) {
	case 8: {
		const uint8_t v = *(uint8_t *)p;
		return (rgb_t) { v, v, v };
	}

	default: UNREACHABLE(pixel_to_rgb);
	}
}

void bitmap_display(const bitmap_t *b) {
	size_t start_y;
	size_t step;

	if (b->pixel_order == BOTTOM_UP) {
		start_y = b->info_hdr.height - 1;
		step = -1;
	} else {
		start_y = 0;
		step = 1;
	}

	uint8_t *row = b->img + b->file_hdr.start_addr + (start_y * b->row_size);
	for (size_t y = 0; y < b->info_hdr.height; y++) {
		for (size_t p = 0; p < b->row_size; p++) {
			const rgb_t c = pixel_to_rgb(row + p, b->info_hdr.bpp);
			rgb_print(c);
		}
		row += (step * b->row_size);
	}

	printf("\n");
}
