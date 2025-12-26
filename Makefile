CFLAGS = -Wall -Wextra -Wpedantic -ggdb

bitmap: main.c bitmap.c bitmap.h
	$(CC) $(CFLAGS) main.c bitmap.c -o bitmap

.PHONY: clean
clean:
	rm -f ./bitmap
