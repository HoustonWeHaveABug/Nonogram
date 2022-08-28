#include <stdio.h>
#include <stdlib.h>

void convert_section(long, long, long, long);

int *cells;

int main(int argc, char *argv[]) {
	char *end;
	int i, j;
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
		for (j = 0; j < width; j++) {
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
	convert_section(width, height, 1L, width);
	convert_section(height, width, width, 1L);
	free(cells);
	return EXIT_SUCCESS;
}

void convert_section(long size_i, long size_j, long weight_i, long weight_j) {
	int sets_n, len, i, j;
	for (i = 0; i < size_i; i++) {
		putchar('\"');
		sets_n = 0;
		len = 0;
		for (j = 0; j < size_j; j++) {
			if (cells[i*weight_i+j*weight_j] == '0') {
				if (len > 0) {
					if (sets_n > 0) {
						putchar(',');
					}
					printf("%d", len);
					sets_n++;
					len = 0;
				}
			}
			else {
				len++;
			}
		}
		if (len > 0) {
			if (sets_n > 0) {
				putchar(',');
			}
			printf("%d", len);
		}
		else {
			if (sets_n == 0) {
				printf("%d", len);
			}
		}
		putchar('\"');
		if (i < size_i-1) {
			putchar(',');
		}
	}
	puts("");
}
