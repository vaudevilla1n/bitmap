CFLAGS = -Wall -Wextra -Wpedantic -ggdb

main: main.c bitmap.c bitmap.h
	$(CC) $(CFLAGS) main.c bitmap.c -o main

.PHONY: clean
clean:
	rm -f ./main
