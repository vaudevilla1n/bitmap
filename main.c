#include "bitmap.h"
#include <stdio.h>

int main(void) {
	bitmap_t *bitmap = bitmap_open("all_gray.bmp");

	if (bitmap_error(bitmap)) {
		bitmap_warn(bitmap);
		return 1;
	}

	bitmap_info(bitmap);
	bitmap_close(bitmap);
}
