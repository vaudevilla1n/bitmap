#include "bitmap.h"
#include <stdio.h>

int main(void) {
	{
		bitmap_t *bitmap = bitmap_open("all_gray.bmp", BITMAP_RD);
		//bitmap_t *bitmap = bitmap_open("broken.bmp", BITMAP_RD);

		if (bitmap_error(bitmap)) {
			bitmap_warn(bitmap);
			return 1;
		}

		bitmap_info(bitmap);
		bitmap_close(bitmap);
	}
	{
		bitmap_t *bitmap = bitmap_open("blackbuck.bmp", BITMAP_RD);

		if (bitmap_error(bitmap)) {
			bitmap_warn(bitmap);
			return 1;
		}

		bitmap_info(bitmap);
		bitmap_display(bitmap);
		bitmap_close(bitmap);
	}
}
