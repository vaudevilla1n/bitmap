#include "bitmap.h"
#include <stdio.h>
#include <stdlib.h>

void usage(void) {
	fprintf(stderr, "usage: bitmap FILE...\n");
	exit(1);
}

int main(int argc, char **argv) {
	if (argc == 1)
		usage();
	
	for (int i = 1; i < argc; i++) {
		bitmap_t *bitmap = bitmap_open(argv[i], BITMAP_RD);

		if (bitmap_error(bitmap)) {
			bitmap_warn(bitmap);
		} else {
			bitmap_info(bitmap);
			bitmap_display(bitmap);
		}

		bitmap_close(bitmap);
	}
}
