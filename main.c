#include "bitmap.h"
#include <stdio.h>
#include <getopt.h>
#include <stdlib.h>

void usage(void) {
	fprintf(stderr, "usage: bitmap [-i (only display info)] FILE...\n");
	exit(1);
}

int main(int argc, char **argv) {
	if (argc == 1)
		usage();

	bool info_only = false;
	
	switch (getopt(argc, argv, "i")) {
	case 'i':
		info_only = true;
		break;
	case -1:
		  break;
	
	default: usage();
	}

	if (optind == argc)
		usage();
	
	for (int i = optind; i < argc; i++) {
		bitmap_t *bitmap = bitmap_open(argv[i], BITMAP_RD);

		if (bitmap_error(bitmap)) {
			bitmap_warn(bitmap);
		} else {
			bitmap_info(bitmap);

			if (!info_only)
				bitmap_display(bitmap);
		}

		bitmap_close(bitmap);
	}
}
