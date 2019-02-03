#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

typedef struct clue_s clue_t;

struct clue_s {
	int pos;
	int sets_n;
	int *sets;
	int len_min;
	clue_t *last;
	clue_t *next;
	int options_n;
};

int read_clue(clue_t *, int, int);
void link_clue(clue_t *, clue_t *, clue_t *);
void nonogram(void);
void choose_column(clue_t *, int, int, int, int);
void try_column_set(clue_t *, int, int, int, int);
void choose_row(clue_t *, int, int, int, int);
void try_row_set(clue_t *, int, int, int, int);
void change_cell(int *, int, int);
void free_clues(clue_t *, int);
void free_clue(clue_t *);

int columns_n, rows_n, *cells, nodes_n, solutions_n, options_min;
clue_t *header;

int main(void) {
	int clues_n, i;
	clue_t *clues;
	if (scanf("%d%d", &columns_n, &rows_n) != 2 || columns_n < 1 || rows_n < 1) {
		fprintf(stderr, "Invalid grid size\n");
		fflush(stderr);
		return EXIT_FAILURE;
	}
	getchar();
	clues_n = columns_n+rows_n;
	clues = malloc(sizeof(clue_t)*(size_t)(clues_n+1));
	if (!clues) {
		fprintf(stderr, "Could not allocate memory for clues\n");
		fflush(stderr);
		return EXIT_FAILURE;
	}
	for (i = 0; i < columns_n-1; i++) {
		if (!read_clue(clues+i, i, ',')) {
			free_clues(clues, i);
			return EXIT_FAILURE;
		}
	}
	if (!read_clue(clues+i, i, '\n')) {
		free_clues(clues, i);
		return EXIT_FAILURE;
	}
	for (i++; i < clues_n-1; i++) {
		if (!read_clue(clues+i, i, ',')) {
			free_clues(clues, i);
			return EXIT_FAILURE;
		}
	}
	if (!read_clue(clues+i, i, '\n')) {
		free_clues(clues, i);
		return EXIT_FAILURE;
	}
	header = clues+clues_n;
	header->options_n = INT_MAX;
	link_clue(clues, header, clues+1);
	for (i = 1; i < clues_n; i++) {
		link_clue(clues+i, clues+i-1, clues+i+1);
	}
	link_clue(header, clues+i-1, clues);
	cells = calloc((size_t)(columns_n*rows_n), sizeof(int));
	if (!cells) {
		fprintf(stderr, "Could not allocate memory for cells\n");
		fflush(stderr);
		return EXIT_FAILURE;
	}
	nodes_n = 0;
	solutions_n = 0;
	nonogram();
	printf("Nodes %d\nSolutions %d\n", nodes_n, solutions_n);
	fflush(stdout);
	free(cells);
	free_clues(clues, clues_n);
	return EXIT_SUCCESS;
}

int read_clue(clue_t *clue, int pos, int separator) {
	int c;
	if (getchar() != '\"') {
		fprintf(stderr, "Double quote expected as clue start\n");
		fflush(stderr);
		return 0;
	}
	clue->pos = pos;
	clue->sets_n = 0;
	do {
		int set;
		if (scanf("%d", &set) != 1 && set < 1) {
			fprintf(stderr, "Invalid set value\n");
			fflush(stderr);
			free_clue(clue);
			return 0;
		}
		if (clue->sets_n == 0) {
			clue->sets = malloc(sizeof(clue_t));
			if (!clue->sets) {
				fprintf(stderr, "Could not allocate memory for clue->sets\n");
				fflush(stderr);
				free_clue(clue);
				return 0;
			}
		}
		else {
			int *sets = realloc(clue->sets, sizeof(clue_t)*(size_t)(clue->sets_n+1));
			if (!sets) {
				fprintf(stderr, "Could not reallocate memory for clue->sets\n");
				fflush(stderr);
				free_clue(clue);
				return 0;
			}
			clue->sets = sets;
		}
		clue->sets[clue->sets_n++] = set;
		clue->len_min += set;
		c = getchar();
		if (c != ',' && c != '\"') {
			fprintf(stderr, "Invalid set separator\n");
			fflush(stderr);
			free_clue(clue);
			return 0;
		}
	}
	while (c != '\"');
	if (clue->sets_n > 0) {
		clue->len_min += clue->sets_n-1;
		if (clue->pos < columns_n) {
			if (clue->len_min > rows_n) {
				fprintf(stderr, "Incompatible clue\n");
				fflush(stderr);
				free_clue(clue);
				return 0;
			}
		}
		else {
			if (clue->len_min > columns_n) {
				fprintf(stderr, "Incompatible clue\n");
				fflush(stderr);
				free_clue(clue);
				return 0;
			}
		}
	}
	if (getchar() != separator) {
		fprintf(stderr, "Invalid clue separator\n");
		fflush(stderr);
		free_clue(clue);
		return 0;
	}
	return 1;
}

void link_clue(clue_t *clue, clue_t *last, clue_t *next) {
	clue->last = last;
	clue->next = next;
}

void nonogram(void) {
	clue_t *clue_min, *clue;
	nodes_n++;
	if (header->next == header) {
		solutions_n++;
		if (solutions_n == 1) {
			int i;
			for (i = 0; i < rows_n; i++) {
				int j;
				for (j = 0; j < columns_n; j++) {
					if (cells[i*columns_n+j] > 0) {
						putchar('*');
					}
					else {
						putchar(' ');
					}
				}
				puts("");
			}
		}
		return;
	}
	clue_min = header;
	options_min = INT_MAX;
	for (clue = header->next; clue != header && clue_min->options_n > 0; clue = clue->next) {
		clue->options_n = 0;
		if (clue->pos < columns_n) {
			choose_column(clue, 0, clue->len_min, 0, 0);
		}
		else {
			choose_row(clue, 0, clue->len_min, 0, 0);
		}
		if (clue->options_n < clue_min->options_n) {
			clue_min = clue;
			options_min = clue_min->options_n;
		}
	}
	if (clue_min->options_n == 0) {
		return;
	}
	clue_min->last->next = clue_min->next;
	clue_min->next->last = clue_min->last;
	if (clue_min->pos < columns_n) {
		choose_column(clue_min, 0, clue_min->len_min, 0, 1);
	}
	else {
		choose_row(clue_min, 0, clue_min->len_min, 0, 1);
	}
	clue_min->next->last = clue_min;
	clue_min->last->next = clue_min;
}

void choose_column(clue_t *clue, int set_idx, int len_min, int pos, int run) {
	int i;
	if (!run && clue->options_n == options_min) {
		return;
	}
	if (set_idx == clue->sets_n) {
		for (i = pos; i < rows_n && cells[i*columns_n+clue->pos] <= 0; i++) {
			change_cell(cells+i*columns_n+clue->pos, 0, -clue->pos-1);
		}
		if (i == rows_n) {
			if (run) {
				nonogram();
			}
			else {
				clue->options_n++;
			}
		}
		for (i--; i >= pos; i--) {
			change_cell(cells+i*columns_n+clue->pos, -clue->pos-1, 0);
		}
		return;
	}
	if (set_idx == 0) {
		try_column_set(clue, set_idx, len_min, pos, run);
		for (i = pos; i < pos+rows_n-len_min && cells[i*columns_n+clue->pos] <= 0; i++) {
			change_cell(cells+i*columns_n+clue->pos, 0, -clue->pos-1);
			try_column_set(clue, set_idx, len_min, i+1, run);
		}
		for (i--; i >= pos; i--) {
			change_cell(cells+i*columns_n+clue->pos, -clue->pos-1, 0);
		}
	}
	else {
		if (cells[pos*columns_n+clue->pos] <= 0) {
			change_cell(cells+pos*columns_n+clue->pos, 0, -clue->pos-1);
			try_column_set(clue, set_idx, len_min, pos+1, run);
			for (i = pos+1; i <= pos+rows_n-len_min && cells[i*columns_n+clue->pos] <= 0; i++) {
				change_cell(cells+i*columns_n+clue->pos, 0, -clue->pos-1);
				try_column_set(clue, set_idx, len_min, i+1, run);
			}
			for (i--; i >= pos; i--) {
				change_cell(cells+i*columns_n+clue->pos, -clue->pos-1, 0);
			}
		}
	}
}

void try_column_set(clue_t *clue, int set_idx, int len_min, int pos, int run) {
	int i;
	for (i = pos; i < clue->sets[set_idx]+pos && cells[i*columns_n+clue->pos] >= 0; i++) {
		change_cell(cells+i*columns_n+clue->pos, 0, clue->pos+1);
	}
	if (i == clue->sets[set_idx]+pos) {
		if (set_idx == 0) {
			choose_column(clue, set_idx+1, len_min-clue->sets[set_idx], i, run);
		}
		else {
			choose_column(clue, set_idx+1, len_min-clue->sets[set_idx]-1, i, run);
		}
	}
	for (i--; i >= pos; i--) {
		change_cell(cells+i*columns_n+clue->pos, clue->pos+1, 0);
	}
}

void choose_row(clue_t *clue, int set_idx, int len_min, int pos, int run) {
	int i;
	if (!run && clue->options_n == options_min) {
		return;
	}
	if (set_idx == clue->sets_n) {
		for (i = pos; i < columns_n && cells[(clue->pos-columns_n)*columns_n+i] <= 0; i++) {
			change_cell(cells+(clue->pos-columns_n)*columns_n+i, 0, -clue->pos-1);
		}
		if (i == columns_n) {
			if (run) {
				nonogram();
			}
			else {
				clue->options_n++;
			}
		}
		for (i--; i >= pos; i--) {
			change_cell(cells+(clue->pos-columns_n)*columns_n+i, -clue->pos-1, 0);
		}
		return;
	}
	if (set_idx == 0) {
		try_row_set(clue, set_idx, len_min, pos, run);
		for (i = pos; i < pos+columns_n-len_min && cells[(clue->pos-columns_n)*columns_n+i] <= 0; i++) {
			change_cell(cells+(clue->pos-columns_n)*columns_n+i, 0, -clue->pos-1);
			try_row_set(clue, set_idx, len_min, i+1, run);
		}
		for (i--; i >= pos; i--) {
			change_cell(cells+(clue->pos-columns_n)*columns_n+i, -clue->pos-1, 0);
		}
	}
	else {
		if (cells[(clue->pos-columns_n)*columns_n+pos] <= 0) {
			change_cell(cells+(clue->pos-columns_n)*columns_n+pos, 0, -clue->pos-1);
			try_row_set(clue, set_idx, len_min, pos+1, run);
			for (i = pos+1; i <= pos+columns_n-len_min && cells[(clue->pos-columns_n)*columns_n+i] <= 0; i++) {
				change_cell(cells+(clue->pos-columns_n)*columns_n+i, 0, -clue->pos-1);
				try_row_set(clue, set_idx, len_min, i+1, run);
			}
			for (i--; i >= pos; i--) {
				change_cell(cells+(clue->pos-columns_n)*columns_n+i, -clue->pos-1, 0);
			}
		}
	}
}

void try_row_set(clue_t *clue, int set_idx, int len_min, int pos, int run) {
	int i;
	for (i = pos; i < clue->sets[set_idx]+pos && cells[(clue->pos-columns_n)*columns_n+i] >= 0; i++) {
		change_cell(cells+(clue->pos-columns_n)*columns_n+i, 0, clue->pos+1);
	}
	if (i == clue->sets[set_idx]+pos) {
		if (set_idx == 0) {
			choose_row(clue, set_idx+1, len_min-clue->sets[set_idx], i, run);
		}
		else {
			choose_row(clue, set_idx+1, len_min-clue->sets[set_idx]-1, i, run);
		}
	}
	for (i--; i >= pos; i--) {
		change_cell(cells+(clue->pos-columns_n)*columns_n+i, clue->pos+1, 0);
	}
}

void change_cell(int *cell, int from, int to) {
	if (*cell == from) {
		*cell = to;
	}
}

void free_clues(clue_t *clues, int clues_max) {
	int i;
	for (i = 0; i < clues_max; i++) {
		free_clue(clues+i);
	}
	free(clues);
}

void free_clue(clue_t *clue) {
	if (clue->sets_n > 0) {
		free(clue->sets);
	}
}
