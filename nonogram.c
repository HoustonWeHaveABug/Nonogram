#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>

#define SEPARATOR_CLUES ','
#define SEPARATOR_LINES '\n'
#define DELIMITER_CLUE '\"'
#define SEPARATOR_COLOR '-'
#define SEPARATOR_SETS ','
#define COLOR_UNKNOWN '?'
#define COLOR_BLACK '*'
#define COLOR_WHITE ' '
#define COLOR_SEVERAL '!'

typedef struct {
	int len;
	int color;
	int pos_min;
}
set_t;

typedef struct clue_s clue_t;

struct clue_s {
	int pos;
	int sets_n;
	set_t *sets;
	int len_min;
	int relaxation;
	int unknown;
	int cache_size;
	int *cache;
	clue_t *last;
	clue_t *next;
};

typedef struct {
	int color;
	int clue_pos;
}
cell_t;

typedef enum {
	STEP_PHASE1,
	STEP_PHASE2_EVAL,
	STEP_PHASE2_RUN
}
step_t;

int read_clue(clue_t *, int, int);
void link_clue(clue_t *, clue_t *, clue_t *);
int nonogram_phase1(void);
void nonogram_phase2(void);
void clear_cache(clue_t *);
int choose_column(clue_t *, int, int, int, step_t, int *);
void set_column_changes(clue_t *, int, int *);
int try_column_set(clue_t *, int, int, int, step_t, int *);
int choose_row(clue_t *, int, int, int, step_t, int *);
void set_row_changes(clue_t *, int, int *);
int try_row_set(clue_t *, int, int, int, step_t, int *);
void change_cell(cell_t *, int, int, int, int);
void set_cell(cell_t *, int, int);
int sum_with_check(int, int);
int color_in_clue(clue_t *, int, int);
void print_grid(void);
void free_clues(int);
void free_clue(clue_t *);

int columns_n, rows_n, colored, clues_n, nodes_n, solutions_n, options_min;
clue_t *clues, *header;
cell_t *cells;

int main(void) {
	int cells_n, changes_n, i;
	if (scanf("%d%d%d", &columns_n, &rows_n, &colored) != 3 || columns_n < 1 || rows_n < 1) {
		fprintf(stderr, "Invalid grid attributes\n");
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
		if (!read_clue(clues+i, i, SEPARATOR_CLUES)) {
			free_clues(i);
			return EXIT_FAILURE;
		}
	}
	if (!read_clue(clues+i, i, SEPARATOR_LINES)) {
		free_clues(i);
		return EXIT_FAILURE;
	}
	for (i++; i < clues_n-1; i++) {
		if (!read_clue(clues+i, i, SEPARATOR_CLUES)) {
			free_clues(i);
			return EXIT_FAILURE;
		}
	}
	if (!read_clue(clues+i, i, SEPARATOR_LINES)) {
		free_clues(i);
		return EXIT_FAILURE;
	}
	header = clues+clues_n;
	link_clue(clues, header, clues+1);
	for (i = 1; i < clues_n; i++) {
		link_clue(clues+i, clues+i-1, clues+i+1);
	}
	link_clue(header, clues+i-1, clues);
	cells_n = (columns_n+1)*(rows_n+1)-1;
	cells = malloc(sizeof(cell_t)*(size_t)cells_n);
	if (!cells) {
		fprintf(stderr, "Could not allocate memory for cells\n");
		fflush(stderr);
		free_clues(clues_n);
		return EXIT_FAILURE;
	}
	for (i = 0; i < cells_n; i++) {
		set_cell(cells+i, COLOR_UNKNOWN, clues_n);
	}
	puts("PHASE 1");
	fflush(stdout);
	do {
		changes_n = nonogram_phase1();
		if (changes_n > 0) {
			print_grid();
			printf("Changes %d\n", changes_n);
			fflush(stdout);
		}
	}
	while (changes_n > 0);
	puts("PHASE 2");
	fflush(stdout);
	nodes_n = 0;
	solutions_n = 0;
	nonogram_phase2();
	printf("Nodes %d\nSolutions %d\n", nodes_n, solutions_n);
	fflush(stdout);
	free(cells);
	free_clues(clues_n);
	return EXIT_SUCCESS;
}

int read_clue(clue_t *clue, int pos, int separator) {
	int c;
	if (getchar() != DELIMITER_CLUE) {
		fprintf(stderr, "Double quote expected as clue start\n");
		fflush(stderr);
		return 0;
	}
	clue->pos = pos;
	clue->sets_n = 0;
	do {
		int set;
		if (scanf("%d", &set) != 1 || set < 0) {
			fprintf(stderr, "Invalid set value\n");
			fflush(stderr);
			if (clue->sets_n > 0) {
				free(clue->sets);
			}
			return 0;
		}
		if (colored) {
			if (getchar() != SEPARATOR_COLOR) {
				fprintf(stderr, "Invalid color separator\n");
				fflush(stderr);
				if (clue->sets_n > 0) {
					free(clue->sets);
				}
				return 0;
			}
			c = getchar();
			if (!isalnum(c)) {
				fprintf(stderr, "Invalid set color\n");
				fflush(stderr);
				if (clue->sets_n > 0) {
					free(clue->sets);
				}
				return 0;
			}
		}
		else {
			c = COLOR_BLACK;
		}
		if (set > 0) {
			if (clue->sets_n == 0) {
				clue->sets = malloc(sizeof(set_t));
				if (!clue->sets) {
					fprintf(stderr, "Could not allocate memory for clue->sets\n");
					fflush(stderr);
					return 0;
				}
			}
			else {
				set_t *sets = realloc(clue->sets, sizeof(set_t)*(size_t)(clue->sets_n+1));
				if (!sets) {
					fprintf(stderr, "Could not reallocate memory for clue->sets\n");
					fflush(stderr);
					free(clue->sets);
					return 0;
				}
				clue->sets = sets;
			}
			clue->sets[clue->sets_n].len = set;
			clue->sets[clue->sets_n].color = c;
			clue->sets_n++;
		}
		c = getchar();
		if (c != SEPARATOR_SETS && c != DELIMITER_CLUE) {
			fprintf(stderr, "Invalid set separator\n");
			fflush(stderr);
			if (clue->sets_n > 0) {
				free(clue->sets);
			}
			return 0;
		}
	}
	while (c != DELIMITER_CLUE);
	if (clue->sets_n == 0) {
		clue->len_min = 0;
	}
	else {
		int i;
		clue->sets[0].pos_min = 0;
		clue->len_min = clue->sets[0].len;
		for (i = 1; i < clue->sets_n; i++) {
			if (clue->sets[i].color == clue->sets[i-1].color) {
				clue->len_min++;
			}
			clue->sets[i].pos_min = clue->len_min;
			clue->len_min += clue->sets[i].len;
		}
	}
	if (clue->pos < columns_n) {
		if (clue->len_min > rows_n) {
			fprintf(stderr, "Incompatible clue\n");
			fflush(stderr);
			if (clue->sets_n > 0) {
				free(clue->sets);
			}
			return 0;
		}
		clue->relaxation = rows_n-clue->len_min;
		clue->unknown = rows_n;
		clue->cache_size = (clue->sets_n+1)*(rows_n+1);
	}
	else {
		if (clue->len_min > columns_n) {
			fprintf(stderr, "Incompatible clue\n");
			fflush(stderr);
			if (clue->sets_n > 0) {
				free(clue->sets);
			}
			return 0;
		}
		clue->relaxation = columns_n-clue->len_min;
		clue->unknown = columns_n;
		clue->cache_size = (clue->sets_n+1)*(columns_n+1);
	}
	clue->cache = malloc(sizeof(int)*(size_t)clue->cache_size);
	if (!clue->cache) {
		fprintf(stderr, "Could not allocate memory for clue->cache\n");
		fflush(stderr);
		if (clue->sets_n > 0) {
			free(clue->sets);
		}
		return 0;
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

int nonogram_phase1(void) {
	int changes_n, step1_first = 0, i;
	clue_t *clue_min, *clue;
	if (header->next == header) {
		return 0;
	}
	clue_min = header->next;
	for (clue = clue_min->next; clue != header; clue = clue->next) {
		if (clue->relaxation < clue_min->relaxation || (clue->relaxation == clue_min->relaxation && clue->unknown < clue_min->unknown)) {
			clue_min = clue;
		}
	}
	if (clue_min->pos < columns_n) {
		printf("Column %d...", clue_min->pos);
	}
	else {
		printf("Row %d...", clue_min->pos-columns_n);
	}
	fflush(stdout);
	clue_min->last->next = clue_min->next;
	clue_min->next->last = clue_min->last;
	changes_n = 0;
	clear_cache(clue_min);
	if (clue_min->pos < columns_n) {
		for (i = 0; i < rows_n; i++) {
			cells[i*(columns_n+1)+columns_n] = cells[i*(columns_n+1)+clue_min->pos];
		}
		choose_column(clue_min, 0, clue_min->len_min, 0, STEP_PHASE1, &step1_first);
		for (i = 0; i < rows_n; i++) {
			if (cells[i*(columns_n+1)+columns_n].color != COLOR_SEVERAL && cells[i*(columns_n+1)+columns_n].clue_pos < clues_n) {
				set_cell(cells+i*(columns_n+1)+clue_min->pos, cells[i*(columns_n+1)+columns_n].color, clues_n);
				clues[clue_min->pos].unknown--;
				clues[i+columns_n].unknown--;
				changes_n++;
			}
		}
	}
	else {
		for (i = 0; i < columns_n; i++) {
			cells[rows_n*(columns_n+1)+i] = cells[(clue_min->pos-columns_n)*(columns_n+1)+i];
		}
		choose_row(clue_min, 0, clue_min->len_min, 0, STEP_PHASE1, &step1_first);
		for (i = 0; i < columns_n; i++) {
			if (cells[rows_n*(columns_n+1)+i].color != COLOR_SEVERAL && cells[rows_n*(columns_n+1)+i].clue_pos < clues_n) {
				set_cell(cells+(clue_min->pos-columns_n)*(columns_n+1)+i, cells[rows_n*(columns_n+1)+i].color, clues_n);
				clues[i].unknown--;
				clues[clue_min->pos].unknown--;
				changes_n++;
			}
		}
	}
	printf(" changes %d\n", changes_n);
	fflush(stdout);
	changes_n += nonogram_phase1();
	clue_min->next->last = clue_min;
	clue_min->last->next = clue_min;
	return changes_n;
}

void nonogram_phase2(void) {
	int step1_first;
	clue_t *clue_min, *clue;
	nodes_n++;
	if (header->next == header) {
		solutions_n++;
		if (solutions_n == 1) {
			print_grid();
		}
		return;
	}
	step1_first = 0;
	clue_min = header;
	options_min = INT_MAX;
	for (clue = header->next; clue != header; clue = clue->next) {
		int options_n;
		clear_cache(clue);
		if (clue->pos < columns_n) {
			options_n = choose_column(clue, 0, clue->len_min, 0, STEP_PHASE2_EVAL, &step1_first);
		}
		else {
			options_n = choose_row(clue, 0, clue->len_min, 0, STEP_PHASE2_EVAL, &step1_first);
		}
		if (options_n < options_min) {
			clue_min = clue;
			options_min = options_n;
		}
		if (options_min == 0) {
			return;
		}
	}
	clue_min->last->next = clue_min->next;
	clue_min->next->last = clue_min->last;
	if (clue_min->pos < columns_n) {
		choose_column(clue_min, 0, clue_min->len_min, 0, STEP_PHASE2_RUN, &step1_first);
	}
	else {
		choose_row(clue_min, 0, clue_min->len_min, 0, STEP_PHASE2_RUN, &step1_first);
	}
	clue_min->next->last = clue_min;
	clue_min->last->next = clue_min;
}

void clear_cache(clue_t *clue) {
	int i;
	for (i = 0; i < clue->cache_size; i++) {
		clue->cache[i] = -1;
	}
}

int choose_column(clue_t *clue, int set_idx, int len_min, int pos, step_t step, int *step1_first) {
	int cache_key = pos*(clue->sets_n+1)+set_idx, r, i;
	if (step == STEP_PHASE1) {
		if (clue->cache[cache_key] >= 0) {
			if (clue->cache[cache_key] > 0) {
				set_column_changes(clue, pos, step1_first);
			}
			return clue->cache[cache_key];
		}
	}
	else if (step == STEP_PHASE2_EVAL) {
		if (clue->cache[cache_key] >= 0) {
			return clue->cache[cache_key];
		}
	}
	else {
		if (clue->cache[cache_key] == 0) {
			return 0;
		}
	}
	if (set_idx == clue->sets_n) {
		for (i = pos; i < rows_n && (cells[i*(columns_n+1)+clue->pos].color == COLOR_WHITE || cells[i*(columns_n+1)+clue->pos].color == COLOR_UNKNOWN); i++) {
			change_cell(cells+i*(columns_n+1)+clue->pos, COLOR_UNKNOWN, clues_n, COLOR_WHITE, clue->pos);
		}
		if (i == rows_n) {
			if (step == STEP_PHASE1) {
				set_column_changes(clue, rows_n, step1_first);
			}
			else if (step == STEP_PHASE2_RUN) {
				nonogram_phase2();
			}
			r = 1;
		}
		else {
			r = 0;
		}
		for (i--; i >= pos; i--) {
			change_cell(cells+i*(columns_n+1)+clue->pos, COLOR_WHITE, clue->pos, COLOR_UNKNOWN, clues_n);
		}
		if (step != STEP_PHASE2_RUN) {
			clue->cache[cache_key] = r;
		}
		return r;
	}
	r = 0;
	if (set_idx == 0 || clue->sets[set_idx].color != clue->sets[set_idx-1].color) {
		r = sum_with_check(r, try_column_set(clue, set_idx, len_min, pos, step, step1_first));
		for (i = pos; i < rows_n-len_min && (cells[i*(columns_n+1)+clue->pos].color == COLOR_WHITE || cells[i*(columns_n+1)+clue->pos].color == COLOR_UNKNOWN) && *step1_first < rows_n && (step != STEP_PHASE2_EVAL || r < options_min); i++) {
			change_cell(cells+i*(columns_n+1)+clue->pos, COLOR_UNKNOWN, clues_n, COLOR_WHITE, clue->pos);
			r = sum_with_check(r, try_column_set(clue, set_idx, len_min, i+1, step, step1_first));
		}
		for (i--; i >= pos; i--) {
			change_cell(cells+i*(columns_n+1)+clue->pos, COLOR_WHITE, clue->pos, COLOR_UNKNOWN, clues_n);
		}
	}
	else {
		if (cells[pos*(columns_n+1)+clue->pos].color == COLOR_WHITE || cells[pos*(columns_n+1)+clue->pos].color == COLOR_UNKNOWN) {
			change_cell(cells+pos*(columns_n+1)+clue->pos, COLOR_UNKNOWN, clues_n, COLOR_WHITE, clue->pos);
			r = sum_with_check(r, try_column_set(clue, set_idx, len_min, pos+1, step, step1_first));
			for (i = pos+1; i <= rows_n-len_min && (cells[i*(columns_n+1)+clue->pos].color == COLOR_WHITE || cells[i*(columns_n+1)+clue->pos].color == COLOR_UNKNOWN) && *step1_first < rows_n && (step != STEP_PHASE2_EVAL || r < options_min); i++) {
				change_cell(cells+i*(columns_n+1)+clue->pos, COLOR_UNKNOWN, clues_n, COLOR_WHITE, clue->pos);
				r = sum_with_check(r, try_column_set(clue, set_idx, len_min, i+1, step, step1_first));
			}
			for (i--; i >= pos; i--) {
				change_cell(cells+i*(columns_n+1)+clue->pos, COLOR_WHITE, clue->pos, COLOR_UNKNOWN, clues_n);
			}
		}
	}
	if (step != STEP_PHASE2_RUN) {
		clue->cache[cache_key] = r;
	}
	return r;
}

void set_column_changes(clue_t *clue, int pos, int *step1_first) {
	int i;
	for (i = *step1_first; i < pos; i++) {
		if (cells[i*(columns_n+1)+clue->pos].clue_pos == clue->pos) {
			if (cells[i*(columns_n+1)+columns_n].color == COLOR_UNKNOWN) {
				set_cell(cells+i*(columns_n+1)+columns_n, cells[i*(columns_n+1)+clue->pos].color, clue->pos);
			}
			else if (cells[i*(columns_n+1)+columns_n].color != COLOR_SEVERAL) {
				if (cells[i*(columns_n+1)+clue->pos].color != cells[i*(columns_n+1)+columns_n].color) {
					set_cell(cells+i*(columns_n+1)+columns_n, COLOR_SEVERAL, clues_n);
				}
			}
		}
	}
	for (; *step1_first < rows_n && cells[*step1_first*(columns_n+1)+columns_n].color != COLOR_UNKNOWN && cells[*step1_first*(columns_n+1)+columns_n].clue_pos == clues_n; *step1_first += 1);
}

int try_column_set(clue_t *clue, int set_idx, int len_min, int pos, step_t step, int *step1_first) {
	int r, i;
	for (i = pos; i < clue->sets[set_idx].len+pos && ((cells[i*(columns_n+1)+clue->pos].color == COLOR_UNKNOWN && color_in_clue(clues+i+columns_n, clue->pos, clue->sets[set_idx].color)) || cells[i*(columns_n+1)+clue->pos].color == clue->sets[set_idx].color); i++) {
		change_cell(cells+i*(columns_n+1)+clue->pos, COLOR_UNKNOWN, clues_n, clue->sets[set_idx].color, clue->pos);
	}
	if (i == clue->sets[set_idx].len+pos) {
		if (set_idx == 0 || clue->sets[set_idx].color != clue->sets[set_idx-1].color) {
			r = choose_column(clue, set_idx+1, len_min-clue->sets[set_idx].len, i, step, step1_first);
		}
		else {
			r = choose_column(clue, set_idx+1, len_min-clue->sets[set_idx].len-1, i, step, step1_first);
		}
	}
	else {
		r = 0;
	}
	for (i--; i >= pos; i--) {
		change_cell(cells+i*(columns_n+1)+clue->pos, clue->sets[set_idx].color, clue->pos, COLOR_UNKNOWN, clues_n);
	}
	return r;
}

int choose_row(clue_t *clue, int set_idx, int len_min, int pos, step_t step, int *step1_first) {
	int cache_key = pos*(clue->sets_n+1)+set_idx, r, i;
	if (step == STEP_PHASE1) {
		if (clue->cache[cache_key] >= 0) {
			if (clue->cache[cache_key] > 0) {
				set_row_changes(clue, pos, step1_first);
			}
			return clue->cache[cache_key];
		}
	}
	else if (step == STEP_PHASE2_EVAL) {
		if (clue->cache[cache_key] >= 0) {
			return clue->cache[cache_key];
		}
	}
	else {
		if (clue->cache[cache_key] == 0) {
			return 0;
		}
	}
	if (set_idx == clue->sets_n) {
		for (i = pos; i < columns_n && (cells[(clue->pos-columns_n)*(columns_n+1)+i].color == COLOR_WHITE || cells[(clue->pos-columns_n)*(columns_n+1)+i].color == COLOR_UNKNOWN); i++) {
			change_cell(cells+(clue->pos-columns_n)*(columns_n+1)+i, COLOR_UNKNOWN, clues_n, COLOR_WHITE, clue->pos);
		}
		if (i == columns_n) {
			if (step == STEP_PHASE1) {
				set_row_changes(clue, columns_n, step1_first);
			}
			else if (step == STEP_PHASE2_RUN) {
				nonogram_phase2();
			}
			r = 1;
		}
		else {
			r = 0;
		}
		for (i--; i >= pos; i--) {
			change_cell(cells+(clue->pos-columns_n)*(columns_n+1)+i, COLOR_WHITE, clue->pos, COLOR_UNKNOWN, clues_n);
		}
		if (step != STEP_PHASE2_RUN) {
			clue->cache[cache_key] = r;
		}
		return r;
	}
	r = 0;
	if (set_idx == 0 || clue->sets[set_idx].color != clue->sets[set_idx-1].color) {
		r = sum_with_check(r, try_row_set(clue, set_idx, len_min, pos, step, step1_first));
		for (i = pos; i < columns_n-len_min && (cells[(clue->pos-columns_n)*(columns_n+1)+i].color == COLOR_WHITE || cells[(clue->pos-columns_n)*(columns_n+1)+i].color == COLOR_UNKNOWN) && *step1_first < columns_n && (step != STEP_PHASE2_EVAL || r < options_min); i++) {
			change_cell(cells+(clue->pos-columns_n)*(columns_n+1)+i, COLOR_UNKNOWN, clues_n, COLOR_WHITE, clue->pos);
			r = sum_with_check(r, try_row_set(clue, set_idx, len_min, i+1, step, step1_first));
		}
		for (i--; i >= pos; i--) {
			change_cell(cells+(clue->pos-columns_n)*(columns_n+1)+i, COLOR_WHITE, clue->pos, COLOR_UNKNOWN, clues_n);
		}
	}
	else {
		if (cells[(clue->pos-columns_n)*(columns_n+1)+pos].color == COLOR_WHITE || cells[(clue->pos-columns_n)*(columns_n+1)+pos].color == COLOR_UNKNOWN) {
			change_cell(cells+(clue->pos-columns_n)*(columns_n+1)+pos, COLOR_UNKNOWN, clues_n, COLOR_WHITE, clue->pos);
			r = sum_with_check(r, try_row_set(clue, set_idx, len_min, pos+1, step, step1_first));
			for (i = pos+1; i <= columns_n-len_min && (cells[(clue->pos-columns_n)*(columns_n+1)+i].color == COLOR_WHITE || cells[(clue->pos-columns_n)*(columns_n+1)+i].color == COLOR_UNKNOWN) && *step1_first < columns_n && (step != STEP_PHASE2_EVAL || r < options_min); i++) {
				change_cell(cells+(clue->pos-columns_n)*(columns_n+1)+i, COLOR_UNKNOWN, clues_n, COLOR_WHITE, clue->pos);
				r = sum_with_check(r, try_row_set(clue, set_idx, len_min, i+1, step, step1_first));
			}
			for (i--; i >= pos; i--) {
				change_cell(cells+(clue->pos-columns_n)*(columns_n+1)+i, COLOR_WHITE, clue->pos, COLOR_UNKNOWN, clues_n);
			}
		}
	}
	if (step != STEP_PHASE2_RUN) {
		clue->cache[cache_key] = r;
	}
	return r;
}

void set_row_changes(clue_t *clue, int pos, int *step1_first) {
	int i;
	for (i = *step1_first; i < pos; i++) {
		if (cells[(clue->pos-columns_n)*(columns_n+1)+i].clue_pos == clue->pos) {
			if (cells[rows_n*(columns_n+1)+i].color == COLOR_UNKNOWN) {
				set_cell(cells+rows_n*(columns_n+1)+i, cells[(clue->pos-columns_n)*(columns_n+1)+i].color, clue->pos);
			}
			else if (cells[rows_n*(columns_n+1)+i].color != COLOR_SEVERAL) {
				if (cells[(clue->pos-columns_n)*(columns_n+1)+i].color != cells[rows_n*(columns_n+1)+i].color) {
					set_cell(cells+rows_n*(columns_n+1)+i, COLOR_SEVERAL, clues_n);
				}
			}
		}
	}
	for (; *step1_first < columns_n && cells[rows_n*(columns_n+1)+*step1_first].color != COLOR_UNKNOWN && cells[rows_n*(columns_n+1)+*step1_first].clue_pos == clues_n; *step1_first += 1);
}

int try_row_set(clue_t *clue, int set_idx, int len_min, int pos, step_t step, int *step1_first) {
	int r, i;
	for (i = pos; i < clue->sets[set_idx].len+pos && ((cells[(clue->pos-columns_n)*(columns_n+1)+i].color == COLOR_UNKNOWN && color_in_clue(clues+i, clue->pos-columns_n, clue->sets[set_idx].color)) || cells[(clue->pos-columns_n)*(columns_n+1)+i].color == clue->sets[set_idx].color); i++) {
		change_cell(cells+(clue->pos-columns_n)*(columns_n+1)+i, COLOR_UNKNOWN, clues_n, clue->sets[set_idx].color, clue->pos);
	}
	if (i == clue->sets[set_idx].len+pos) {
		if (set_idx == 0 || clue->sets[set_idx].color != clue->sets[set_idx-1].color) {
			r = choose_row(clue, set_idx+1, len_min-clue->sets[set_idx].len, i, step, step1_first);
		}
		else {
			r = choose_row(clue, set_idx+1, len_min-clue->sets[set_idx].len-1, i, step, step1_first);
		}
	}
	else {
		r = 0;
	}
	for (i--; i >= pos; i--) {
		change_cell(cells+(clue->pos-columns_n)*(columns_n+1)+i, clue->sets[set_idx].color, clue->pos, COLOR_UNKNOWN, clues_n);
	}
	return r;
}

void change_cell(cell_t *cell, int from_color, int from_clue_pos, int to_color, int to_clue_pos) {
	if (cell->color == from_color && cell->clue_pos == from_clue_pos) {
		set_cell(cell, to_color, to_clue_pos);
	}
}

void set_cell(cell_t *cell, int color, int clue_pos) {
	cell->color = color;
	cell->clue_pos = clue_pos;
}

int sum_with_check(int a, int b) {
	if (a <= INT_MAX-b) {
		return a+b;
	}
	return INT_MAX;
}

int color_in_clue(clue_t *clue, int pos, int color) {
	int i;
	for (i = 0; i < clue->sets_n && clue->sets[i].pos_min <= pos; i++) {
		if (clue->sets[i].pos_min+clue->sets[i].len+clue->relaxation > pos && clue->sets[i].color == color) {
			return 1;
		}
	}
	return 0;
}

void print_grid(void) {
	int i;
	for (i = 0; i < rows_n; i++) {
		int j;
		for (j = 0; j < columns_n; j++) {
			putchar(cells[i*(columns_n+1)+j].color);
		}
		puts("");
	}
}

void free_clues(int clues_max) {
	int i;
	for (i = 0; i < clues_max; i++) {
		free_clue(clues+i);
	}
	free(clues);
}

void free_clue(clue_t *clue) {
	free(clue->cache);
	if (clue->sets_n > 0) {
		free(clue->sets);
	}
}
