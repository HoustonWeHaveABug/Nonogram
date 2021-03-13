#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
	char *end;
	int *cells, len, i, j;
	long width, height;
	if (argc != 3) {
		fprintf(stderr, "Usage: %s width height\n", argv[0]);
		fflush(stderr);
		return EXIT_FAILURE;
	}
	width = strtol(argv[1], &end, 10);
	if (*end || width < 1) {
		fprintf(stderr, "Invalid width\n");
		fflush(stderr);
		return EXIT_FAILURE;
	}
	height = strtol(argv[2], &end, 10);
	if (*end || height < 1) {
		fprintf(stderr, "Invalid height\n");
		fflush(stderr);
		return EXIT_FAILURE;
	}
	cells = malloc(sizeof(int)*(size_t)(width*height));
	if (!cells) {
		fprintf(stderr, "Could not allocate memory for cells\n");
		fflush(stderr);
		return EXIT_FAILURE;
	}
	for (i = 0; i < height; i++) {
		for (j = 0; j < height; j++) {
			cells[i*width+j] = getchar();
			if (cells[i*width+j] != '0' && cells[i*width+j] != '1') {
				fprintf(stderr, "Invalid cell (row %d column %d)\n", i, j);
				fflush(stderr);
				free(cells);
				return EXIT_FAILURE;
			}
		}
		if (getchar() != '\n') {
			fprintf(stderr, "End of line expected\n");
			fflush(stderr);
			free(cells);
			return EXIT_FAILURE;
		}
	}
	printf("%ld\n%ld\n0\n", width, height);
	for (i = 0; i < width; i++) {
		putchar('\"');
		len = 0;
		for (j = 0; j < height; j++) {
			if (cells[j*width+i] == '0') {
				if (len > 0) {
					printf("%d,", len);
					len = 0;
				}
			}
			else {
				len++;
				if (j == height-1) {
					printf("%d,", len);
				}
			}
		}
		printf("0\"");
		if (i < width-1) {
			putchar(',');
		}
	}
	puts("");
	for (i = 0; i < height; i++) {
		putchar('\"');
		len = 0;
		for (j = 0; j < width; j++) {
			if (cells[i*width+j] == '0') {
				if (len > 0) {
					printf("%d,", len);
					len = 0;
				}
			}
			else {
				len++;
				if (j == width-1) {
					printf("%d,", len);
				}
			}
		}
		printf("0\"");
		if (i < height-1) {
			putchar(',');
		}
	}
	puts("");
	free(cells);
	return EXIT_SUCCESS;
}
