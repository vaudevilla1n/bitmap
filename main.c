#include "bitmap.h"
#include <stdio.h>

int main(void) {
	bitmap_err_t err;
	bitmap_t *bitmap = bitmap_open("all_gray.bmp", &err);

	if (err != BITMAP_OK) {
		bitmap_error("all_gray.bmp", err);
		return 1;
	}

	bitmap_info(bitmap);
	bitmap_close(bitmap);
}
