#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>

#define SEPARATOR_CLUES ','
#define SEPARATOR_LINES '\n'
#define DELIMITER_CLUE '\"'
#define SEPARATOR_COLOR '-'
#define SEPARATOR_SETS ','
#define COLOR_UNKNOWN '?'
#define COLOR_BLACK '*'
#define COLOR_EMPTY ' '
#define COLOR_SEVERAL '!'
#define CLUE_POS_UNKNOWN -1
#define BOUNDS_CUR 0
#define BOUNDS_SWP 1
#define BOUNDS_BCK 2
#define BOUNDS_SIZE 3
#define CACHE_EMPTY 0

typedef struct {
	int cell_pos;
	int val;
}
option_t;

typedef struct set_s set_t;
typedef struct clue_s clue_t;
typedef struct cell_s cell_t;

struct set_s {
	int len;
	int color;
	int empty_before;
	int *empty_bounds_min;
	int *empty_bounds_max;
	int *color_bounds_min;
	int *color_bounds_max;
	int empty_cache_size;
	int *empty_cache;
	int color_cache_size;
	int *color_cache;
	int cross_cache_size;
	int *cross_cache;
	int options_n;
	option_t *options;
	set_t *last;
	set_t *next;
};

struct clue_s {
	int pos;
	int sets_n;
	set_t *sets;
	set_t *sets_header;
	int bounds_size;
	int relaxation;
	int sweeped;
	cell_t *cells_header;
	clue_t *last;
	clue_t *next;
};

struct cell_s {
	int row;
	int column;
	int color;
	int clue_pos;
	cell_t *column_last;
	cell_t *column_next;
	cell_t *row_last;
	cell_t *row_next;
};

int read_clue(clue_t *, int, int);
void init_set_fields(set_t *, int, int);
int finish_init_clue_sets(clue_t *);
int init_set_tables(set_t *, int, int, int, int, int, int);
void link_set(set_t *, set_t *, set_t *);
void link_clue(clue_t *, clue_t *, clue_t *);
void link_column_cell(cell_t *, cell_t *, cell_t *);
void link_row_cell(cell_t *, cell_t *, cell_t *);
int nonogram(int, int, int, int, int);
int reallocate_bounds(set_t *, int);
int compare_clues(const void *, const void *);
void clear_positive_empty_cache(set_t *);
void clear_positive_color_cache(set_t *);
void clear_positive_cross_cache(set_t *);
void reset_clue_bounds(clue_t *, int);
int sweep_column(int, clue_t *, set_t *, int);
int sweep_column_empty_then_set(int, clue_t *, set_t *, int, int *);
int sweep_column_set(int, clue_t *, set_t *, int, int *);
void check_column_cell(clue_t *, cell_t *, int);
int sweep_row(int, clue_t *, set_t *, int);
int sweep_row_empty_then_set(int, clue_t *, set_t *, int, int *);
int sweep_row_set(int, clue_t *, set_t *, int, int *);
void check_row_cell(clue_t *, cell_t *, int);
int sum_with_limit(int, int);
int match_crossed_clue(int, clue_t *, int, set_t *, int);
void cover_cell(cell_t *);
void cover_clue(clue_t *);
void cover_set(set_t *);
int compare_sets(const void *, const void *);
void evaluate_set(int, int, int, int, int, set_t *, int *, int *, int *);
void uncover_set(set_t *);
void uncover_clue(clue_t *);
void uncover_cell(cell_t *);
void clear_negative_empty_cache(int, set_t *);
void clear_negative_color_cache(int, set_t *);
void clear_negative_cross_cache(int, set_t *);
void init_empty_bounds(set_t *, int, int, int);
void init_color_bounds(set_t *, int, int, int);
int change_cell(cell_t *, int, int, int, int);
void init_cell(cell_t *, int, int);
void free_clues(int);
void free_clue(clue_t *);
void free_set(set_t *);

int width, height, colored, columns_n, clues_n, grid_size, nodes_n;
long solutions_n_max, solutions_n;
unsigned time_zero;
set_t **locked_sets, **sorted_sets;
clue_t *clues, *header, **sorted_clues, **locked_clues;
cell_t *cells, **locked_cells;

int main(int argc, char *argv[]) {
	char *end;
	int cells_n, sets_n, i, j;
	if (argc > 2) {
		fprintf(stderr, "Usage: %s [maximum number of solutions]\n", argv[0]);
		fflush(stderr);
		return EXIT_FAILURE;
	}
	if (argc == 1) {
		solutions_n_max = LONG_MAX;
	}
	else {
		solutions_n_max = strtol(argv[1], &end, 10);
		if (solutions_n_max < 1) {
			fprintf(stderr, "Invalid maximum number of solutions\n");
			fflush(stderr);
			return EXIT_FAILURE;
		}
	}
	if (scanf("%d%d%d", &width, &height, &colored) != 3 || width < 1 || height < 1) {
		fprintf(stderr, "Invalid grid attributes\n");
		fflush(stderr);
		return EXIT_FAILURE;
	}
	getchar();
	columns_n = width+1;
	clues_n = width+height;
	grid_size = width*height;
	clues = malloc(sizeof(clue_t)*(size_t)(clues_n+1));
	if (!clues) {
		fprintf(stderr, "Could not allocate memory for clues\n");
		fflush(stderr);
		return EXIT_FAILURE;
	}
	for (i = 0; i < width-1; i++) {
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
	for (i = 0; i < clues_n; i++) {
		if (!finish_init_clue_sets(clues+i)) {
			free_clues(clues_n);
			return EXIT_FAILURE;
		}
	}
	header = clues+clues_n;
	link_clue(clues, header, clues+1);
	for (i = 1; i < clues_n; i++) {
		link_clue(clues+i, clues+i-1, clues+i+1);
	}
	link_clue(header, header-1, clues);
	cells_n = columns_n*(height+1)-1;
	cells = malloc(sizeof(cell_t)*(size_t)(cells_n+clues_n));
	if (!cells) {
		fprintf(stderr, "Could not allocate memory for cells\n");
		fflush(stderr);
		free_clues(clues_n);
		return EXIT_FAILURE;
	}
	for (i = 0; i < width; i++) {
		for (j = 0; j < height; j++) {
			cells[j*columns_n+i].column = i;
			cells[j*columns_n+i].row = j;
			init_cell(cells+j*columns_n+i, COLOR_UNKNOWN, CLUE_POS_UNKNOWN);
		}
	}
	for (i = 0; i < width; i++) {
		clues[i].cells_header = cells+cells_n+i;
		link_column_cell(cells+i, clues[i].cells_header, cells+columns_n+i);
		for (j = 1; j < height-1; j++) {
			link_column_cell(cells+j*columns_n+i, cells+(j-1)*columns_n+i, cells+(j+1)*columns_n+i);
		}
		link_column_cell(cells+j*columns_n+i, cells+(j-1)*columns_n+i, clues[i].cells_header);
		link_column_cell(clues[i].cells_header, cells+j*columns_n+i, cells+i);
	}
	for (i = 0; i < height; i++) {
		clues[width+i].cells_header = cells+cells_n+width+i;
		link_row_cell(cells+i*columns_n, clues[width+i].cells_header, cells+i*columns_n+1);
		for (j = 1; j < width-1; j++) {
			link_row_cell(cells+i*columns_n+j, cells+i*columns_n+j-1, cells+i*columns_n+j+1);
		}
		link_row_cell(cells+i*columns_n+j, cells+i*columns_n+j-1, clues[width+i].cells_header);
		link_row_cell(clues[width+i].cells_header, cells+i*columns_n+j, cells+i*columns_n);
	}
	sets_n = clues[0].sets_n;
	for (i = 1; i < clues_n; i++) {
		sets_n += clues[i].sets_n;
	}
	if (sets_n > 0) {
		locked_sets = malloc(sizeof(set_t *)*(size_t)sets_n);
		if (!locked_sets) {
			fprintf(stderr, "Could not allocate memory for locked_sets\n");
			fflush(stderr);
			free_clues(clues_n);
			return EXIT_FAILURE;
		}
		sorted_sets = malloc(sizeof(set_t *)*(size_t)sets_n);
		if (!sorted_sets) {
			fprintf(stderr, "Could not allocate memory for sorted_sets\n");
			fflush(stderr);
			free(locked_sets);
			free(cells);
			free_clues(clues_n);
			return EXIT_FAILURE;
		}
	}
	sorted_clues = malloc(sizeof(clue_t *)*(size_t)clues_n);
	if (!sorted_clues) {
		fprintf(stderr, "Could not allocate memory for sorted_clues\n");
		fflush(stderr);
		if (sets_n > 0) {
			free(sorted_sets);
			free(locked_sets);
		}
		free(cells);
		free_clues(clues_n);
		return EXIT_FAILURE;
	}
	locked_clues = malloc(sizeof(clue_t *)*(size_t)clues_n);
	if (!locked_clues) {
		fprintf(stderr, "Could not allocate memory for locked_clues\n");
		fflush(stderr);
		free(sorted_clues);
		if (sets_n > 0) {
			free(sorted_sets);
			free(locked_sets);
		}
		free(cells);
		free_clues(clues_n);
		return EXIT_FAILURE;
	}
	locked_cells = malloc(sizeof(cell_t *)*(size_t)grid_size);
	if (!locked_cells) {
		fprintf(stderr, "Could not allocate memory for locked_cells\n");
		fflush(stderr);
		free(locked_clues);
		free(sorted_clues);
		if (sets_n > 0) {
			free(sorted_sets);
			free(locked_sets);
		}
		free(cells);
		free_clues(clues_n);
		return EXIT_FAILURE;
	}
	time_zero = (unsigned)time(NULL);
	nodes_n = 0;
	solutions_n = 0;
	nonogram(0, 0, 0, 0, 1);
	printf("Time %us\nNodes %d\nSolutions %ld\n", (unsigned)time(NULL)-time_zero, nodes_n, solutions_n);
	fflush(stdout);
	free(locked_cells);
	free(locked_clues);
	free(sorted_clues);
	if (sets_n > 0) {
		free(sorted_sets);
		free(locked_sets);
	}
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
	clue->sets = malloc(sizeof(set_t));
	if (!clue->sets) {
		fprintf(stderr, "Could not allocate memory for clue->sets\n");
		fflush(stderr);
		return 0;
	}
	do {
		int len;
		if (scanf("%d", &len) != 1 || len < 0) {
			fprintf(stderr, "Invalid set length\n");
			fflush(stderr);
			free(clue->sets);
			return 0;
		}
		if (colored && len > 0) {
			if (getchar() != SEPARATOR_COLOR) {
				fprintf(stderr, "Invalid color separator\n");
				fflush(stderr);
				free(clue->sets);
				return 0;
			}
			c = getchar();
			if (!isalnum(c)) {
				fprintf(stderr, "Invalid set color\n");
				fflush(stderr);
				free(clue->sets);
				return 0;
			}
		}
		else {
			c = COLOR_BLACK;
		}
		if (len > 0) {
			set_t *sets = realloc(clue->sets, sizeof(set_t)*(size_t)(clue->sets_n+2));
			if (!sets) {
				fprintf(stderr, "Could not reallocate memory for clue->sets\n");
				fflush(stderr);
				free(clue->sets);
				return 0;
			}
			clue->sets = sets;
			init_set_fields(clue->sets+clue->sets_n, len, c);
			clue->sets_n++;
		}
		c = getchar();
		if (c != SEPARATOR_SETS && c != DELIMITER_CLUE) {
			fprintf(stderr, "Invalid set separator\n");
			fflush(stderr);
			free(clue->sets);
			return 0;
		}
	}
	while (c != DELIMITER_CLUE);
	clue->sets_header = clue->sets+clue->sets_n;
	init_set_fields(clue->sets_header, 0, COLOR_UNKNOWN);
	clue->bounds_size = BOUNDS_SIZE;
	if (clue->sets_n == 0) {
		if (clue->pos < width) {
			clue->relaxation = height;
		}
		else {
			clue->relaxation = width;
		}
	}
	else {
		int len_min = clue->sets[0].len, i;
		clue->sets[0].empty_before = 0;
		for (i = 1; i <= clue->sets_n; i++) {
			if (clue->sets[i].color == clue->sets[i-1].color) {
				len_min++;
				clue->sets[i].empty_before = 1;
			}
			else {
				clue->sets[i].empty_before = 0;
			}
			len_min += clue->sets[i].len;
		}
		if (clue->pos < width) {
			if (len_min > height) {
				fprintf(stderr, "Incompatible clue\n");
				fflush(stderr);
				free(clue->sets);
				return 0;
			}
			clue->relaxation = height-len_min;
		}
		else {
			if (len_min > width) {
				fprintf(stderr, "Incompatible clue\n");
				fflush(stderr);
				free(clue->sets);
				return 0;
			}
			clue->relaxation = width-len_min;
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

void init_set_fields(set_t *set, int len, int color) {
	set->len = len;
	set->color = color;
	set->empty_bounds_min = NULL;
	set->empty_bounds_max = NULL;
	set->color_bounds_min = NULL;
	set->color_bounds_max = NULL;
	set->empty_cache = NULL;
	set->color_cache = NULL;
	set->cross_cache = NULL;
	set->options = NULL;
}

int finish_init_clue_sets(clue_t *clue) {
	int len_min = 0, i;
	if (clue->sets_n > 0) {
		if (!init_set_tables(clue->sets, len_min, len_min, len_min, len_min+clue->relaxation, clue->relaxation+1, clue->relaxation+1+clue->sets[0].len)) {
			return 0;
		}
		link_set(clue->sets, clue->sets_header, clue->sets+1);
		len_min += clue->sets[0].len;
		for (i = 1; i < clue->sets_n; i++) {
			if (!init_set_tables(clue->sets+i, len_min, len_min+clue->relaxation, len_min, len_min+clue->relaxation, clue->relaxation+1, clue->relaxation+1+clue->sets[i].len)) {
				return 0;
			}
			link_set(clue->sets+i, clue->sets+i-1, clue->sets+i+1);
			len_min += clue->sets[i].len+clue->sets[i].empty_before;
		}
	}
	if (clue->pos < width) {
		if (!init_set_tables(clue->sets_header, len_min, height, height, height, 0, 0)) {
			return 0;
		}
		init_empty_bounds(clue->sets_header, BOUNDS_SWP, len_min, height);
		init_color_bounds(clue->sets_header, BOUNDS_SWP, height, height);
		init_empty_bounds(clue->sets_header, BOUNDS_BCK, len_min, height);
		init_color_bounds(clue->sets_header, BOUNDS_BCK, height, height);
	}
	else {
		if (!init_set_tables(clue->sets_header, len_min, width, width, width, 0, 0)) {
			return 0;
		}
		init_empty_bounds(clue->sets_header, BOUNDS_SWP, len_min, width);
		init_color_bounds(clue->sets_header, BOUNDS_SWP, width, width);
		init_empty_bounds(clue->sets_header, BOUNDS_BCK, len_min, width);
		init_color_bounds(clue->sets_header, BOUNDS_BCK, width, width);
	}
	if (clue->sets_n > 0) {
		link_set(clue->sets_header, clue->sets_header-1, clue->sets);
	}
	else {
		link_set(clue->sets_header, clue->sets_header, clue->sets_header);
	}
	return 1;
}

int init_set_tables(set_t *set, int empty_bound_min, int empty_bound_max, int color_bound_min, int color_bound_max, int color_cache_size, int cross_cache_size) {
	int i;
	set->empty_bounds_min = malloc(sizeof(int)*(size_t)BOUNDS_SIZE);
	if (!set->empty_bounds_min) {
		fprintf(stderr, "Could not allocate memory for set->empty_bounds_min\n");
		fflush(stderr);
		return 0;
	}
	set->empty_bounds_max = malloc(sizeof(int)*(size_t)BOUNDS_SIZE);
	if (!set->empty_bounds_max) {
		fprintf(stderr, "Could not allocate memory for set->empty_bounds_max\n");
		fflush(stderr);
		free_set(set);
		return 0;
	}
	init_empty_bounds(set, BOUNDS_CUR, empty_bound_min, empty_bound_max);
	set->color_bounds_min = malloc(sizeof(int)*(size_t)BOUNDS_SIZE);
	if (!set->color_bounds_min) {
		fprintf(stderr, "Could not allocate memory for set->color_bounds_min\n");
		fflush(stderr);
		free_set(set);
		return 0;
	}
	set->color_bounds_max = malloc(sizeof(int)*(size_t)BOUNDS_SIZE);
	if (!set->color_bounds_max) {
		fprintf(stderr, "Could not allocate memory for set->color_bounds_max\n");
		fflush(stderr);
		free_set(set);
		return 0;
	}
	init_color_bounds(set, BOUNDS_CUR, color_bound_min, color_bound_max);
	set->empty_cache_size = empty_bound_max-empty_bound_min+1;
	set->empty_cache = malloc(sizeof(int)*(size_t)set->empty_cache_size);
	if (!set->empty_cache) {
		fprintf(stderr, "Could not allocate memory for set->empty_cache\n");
		fflush(stderr);
		free_set(set);
		return 0;
	}
	for (i = 0; i < set->empty_cache_size; i++) {
		set->empty_cache[i] = CACHE_EMPTY;
	}
	set->color_cache_size = color_cache_size;
	if (color_cache_size > 0) {
		set->color_cache = malloc(sizeof(int)*(size_t)color_cache_size);
		if (!set->color_cache) {
			fprintf(stderr, "Could not allocate memory for set->color_cache\n");
			fflush(stderr);
			free_set(set);
			return 0;
		}
		for (i = 0; i < color_cache_size; i++) {
			set->color_cache[i] = CACHE_EMPTY;
		}
	}
	set->cross_cache_size = cross_cache_size;
	if (cross_cache_size > 0) {
		set->cross_cache = malloc(sizeof(int)*(size_t)cross_cache_size);
		if (!set->cross_cache) {
			fprintf(stderr, "Could not allocate memory for set->cross_cache\n");
			fflush(stderr);
			free_set(set);
			return 0;
		}
		for (i = 0; i < cross_cache_size; i++) {
			set->cross_cache[i] = CACHE_EMPTY;
		}
	}
	if (color_cache_size > 0) {
		set->options = malloc(sizeof(option_t)*(size_t)color_cache_size);
		if (!set->options) {
			fprintf(stderr, "Could not allocate memory for set->options\n");
			fflush(stderr);
			free_set(set);
			return 0;
		}
	}
	return 1;
}

void link_set(set_t *set, set_t *last, set_t *next) {
	set->last = last;
	set->next = next;
}

void link_clue(clue_t *clue, clue_t *last, clue_t *next) {
	clue->last = last;
	clue->next = next;
}

void link_column_cell(cell_t *cell, cell_t *last, cell_t *next) {
	cell->column_last = last;
	cell->column_next = next;
}

void link_row_cell(cell_t *cell, cell_t *last, cell_t *next) {
	cell->row_last = last;
	cell->row_next = next;
}

int nonogram(int depth, int locked_sets_sum, int locked_clues_sum, int locked_cells_sum, int run) {
	int locked_cells_n = 0, locked_clues_n = 0, locked_sets_n = 0, changes_n, clue_options_n_min, r, i, j;
	set_t *set;
	clue_t *clue;
	if (solutions_n == solutions_n_max) {
		return -grid_size;
	}
	nodes_n++;
	for (clue = header->next; clue != header; clue = clue->next) {
		if (clue->bounds_size < depth+BOUNDS_SIZE) {
			for (i = 0; i < clue->sets_n; i++) {
				if (!reallocate_bounds(clue->sets+i, depth+BOUNDS_SIZE)) {
					return -grid_size;
				}
			}
			clue->bounds_size = depth+BOUNDS_SIZE;
		}
		clue->sweeped = 0;
		for (i = 0; i < clue->sets_n; i++) {
			init_empty_bounds(clue->sets+i, depth+BOUNDS_BCK, clue->sets[i].empty_bounds_min[BOUNDS_CUR], clue->sets[i].empty_bounds_max[BOUNDS_CUR]);
			init_color_bounds(clue->sets+i, depth+BOUNDS_BCK, clue->sets[i].color_bounds_min[BOUNDS_CUR], clue->sets[i].color_bounds_max[BOUNDS_CUR]);
		}
	}
	do {
		int sorted_clues_n = 0;
		changes_n = 0;
		clue_options_n_min = INT_MAX;
		for (clue = header->next; clue != header; clue = clue->next) {
			sorted_clues[sorted_clues_n++] = clue;
		}
		qsort(sorted_clues, (size_t)sorted_clues_n, sizeof(clue_t *), compare_clues);
		for (clue = header->next; clue != header; clue = clue->next) {
			clue->sweeped = 0;
		}
		for (i = 0; i < sorted_clues_n && clue_options_n_min > 0; i++) {
			int clue_options_n;
			cell_t *cell;
			clue = sorted_clues[i];
			for (set = clue->sets_header->next; set != clue->sets_header; set = set->next) {
				init_empty_bounds(set, BOUNDS_SWP, set->empty_bounds_min[BOUNDS_CUR], set->empty_bounds_max[BOUNDS_CUR]);
				init_color_bounds(set, BOUNDS_SWP, set->color_bounds_min[BOUNDS_CUR], set->color_bounds_max[BOUNDS_CUR]);
			}
			for (j = 0; j < clue->sets_n; j++) {
				clear_positive_empty_cache(clue->sets+j);
				clear_positive_color_cache(clue->sets+j);
				clear_positive_cross_cache(clue->sets+j);
			}
			clear_positive_empty_cache(clue->sets+j);
			if (clue->pos < width) {
				reset_clue_bounds(clue, height);
				for (cell = clue->cells_header->column_next; cell != clue->cells_header; cell = cell->column_next) {
					init_cell(cells+cell->row*columns_n+width, COLOR_UNKNOWN, CLUE_POS_UNKNOWN);
				}
				clue_options_n = sweep_column(depth, clue, clue->sets, 0);
				for (cell = clue->cells_header->column_next; cell != clue->cells_header; cell = cell->column_next) {
					if (cells[cell->row*columns_n+width].clue_pos == clue->pos) {
						clue->sweeped++;
						clues[width+cell->row].sweeped++;
						init_cell(cell, cells[cell->row*columns_n+width].color, clues_n);
						cover_cell(cell);
						locked_cells[locked_cells_sum+locked_cells_n] = cell;
						locked_cells_n++;
						changes_n++;
					}
				}
			}
			else {
				reset_clue_bounds(clue, width);
				for (cell = clue->cells_header->row_next; cell != clue->cells_header; cell = cell->row_next) {
					init_cell(cells+height*columns_n+cell->column, COLOR_UNKNOWN, CLUE_POS_UNKNOWN);
				}
				clue_options_n = sweep_row(depth, clue, clue->sets, 0);
				for (cell = clue->cells_header->row_next; cell != clue->cells_header; cell = cell->row_next) {
					if (cells[height*columns_n+cell->column].clue_pos == clue->pos) {
						clues[cell->column].sweeped++;
						clue->sweeped++;
						init_cell(cell, cells[height*columns_n+cell->column].color, clues_n);
						cover_cell(cell);
						locked_cells[locked_cells_sum+locked_cells_n] = cell;
						locked_cells_n++;
						changes_n++;
					}
				}
			}
			if (clue_options_n == 1) {
				cover_clue(clue);
				locked_clues[locked_clues_sum+locked_clues_n] = clue;
				locked_clues_n++;
			}
			else if (clue_options_n < clue_options_n_min) {
				clue_options_n_min = clue_options_n;
			}
			for (set = clue->sets_header->next; set != clue->sets_header; set = set->next) {
				if (set->color_bounds_min[BOUNDS_CUR] == set->color_bounds_max[BOUNDS_CUR]) {
					init_color_bounds(set, BOUNDS_SWP, set->color_bounds_min[BOUNDS_CUR], set->color_bounds_max[BOUNDS_CUR]);
					cover_set(set);
					locked_sets[locked_sets_sum+locked_sets_n] = set;
					locked_sets_n++;
				}
			}
		}
	}
	while (changes_n > 0 && clue_options_n_min > 0);
	if (clue_options_n_min > 0) {
		if (run) {
			if (header->next == header) {
				solutions_n++;
				printf("Time %us\nNodes %d\n", (unsigned)time(NULL)-time_zero, nodes_n);
				for (i = 0; i < height; i++) {
					for (j = 0; j < width; j++) {
						putchar(cells[i*columns_n+j].color);
					}
					puts("");
				}
				fflush(stdout);
			}
			else {
				int sorted_sets_n = 0, set_options_n_min = INT_MAX, set_solutions_n_max = -1, set_vals_sum_max = -1;
				set_t *set_min = NULL;
				for (clue = header->next; clue != header; clue = clue->next) {
					for (set = clue->sets_header->next; set != clue->sets_header; set = set->next) {
						set->options_n = 0;
						for (i = set->color_bounds_min[BOUNDS_CUR]; i <= set->color_bounds_max[BOUNDS_CUR]; i++) {
							if (set->color_cache[i-set->color_bounds_min[BOUNDS_BCK]] > CACHE_EMPTY) {
								set->options[set->options_n++].cell_pos = i;
							}
						}
						sorted_sets[sorted_sets_n++] = set;
					}
				}
				qsort(sorted_sets, (size_t)sorted_sets_n, sizeof(set_t *), compare_sets);
				for (i = 0; i < sorted_sets_n && set_options_n_min > 0; i++) {
					int set_options_n, set_solutions_n, set_vals_sum;
					evaluate_set(depth+1, locked_sets_sum+locked_sets_n, locked_clues_sum+locked_clues_n, locked_cells_sum+locked_cells_n, set_options_n_min, sorted_sets[i], &set_options_n, &set_solutions_n, &set_vals_sum);
					if (set_options_n < set_options_n_min || (set_options_n == set_options_n_min && (set_solutions_n > set_solutions_n_max || (set_solutions_n == set_solutions_n_max && set_vals_sum > set_vals_sum_max)))) {
						set_min = sorted_sets[i];
						set_options_n_min = set_options_n;
						set_solutions_n_max = set_solutions_n;
						set_vals_sum_max = set_vals_sum;
					}
				}
				for (i = 0; i < set_min->options_n; i++) {
					set_min->color_cache[set_min->options[i].cell_pos-set_min->color_bounds_min[BOUNDS_BCK]] = -depth*2-2;
				}
				for (i = 0; i < set_min->options_n; i++) {
					if (set_min->options[i].val >= 0) {
						set_min->color_cache[set_min->options[i].cell_pos-set_min->color_bounds_min[BOUNDS_BCK]] = CACHE_EMPTY;
						nonogram(depth+1, locked_sets_sum+locked_sets_n, locked_clues_sum+locked_clues_n, locked_cells_sum+locked_cells_n, 1);
						set_min->color_cache[set_min->options[i].cell_pos-set_min->color_bounds_min[BOUNDS_BCK]] = -depth*2-2;
					}
				}
				for (i = 0; i < set_min->options_n; i++) {
					set_min->color_cache[set_min->options[i].cell_pos-set_min->color_bounds_min[BOUNDS_BCK]] = CACHE_EMPTY;
				}
			}
		}
		r = locked_cells_n;
	}
	else {
		r = -grid_size;
	}
	while (locked_sets_n > 0) {
		locked_sets_n--;
		uncover_set(locked_sets[locked_sets_sum+locked_sets_n]);
	}
	while (locked_clues_n > 0) {
		locked_clues_n--;
		uncover_clue(locked_clues[locked_clues_sum+locked_clues_n]);
	}
	while (locked_cells_n > 0) {
		locked_cells_n--;
		uncover_cell(locked_cells[locked_cells_sum+locked_cells_n]);
		init_cell(locked_cells[locked_cells_sum+locked_cells_n], COLOR_UNKNOWN, CLUE_POS_UNKNOWN);
	}
	for (clue = header->next; clue != header; clue = clue->next) {
		for (i = 0; i < clue->sets_n; i++) {
			clear_negative_cross_cache(depth, clue->sets+i);
			clear_negative_color_cache(depth, clue->sets+i);
			clear_negative_empty_cache(depth, clue->sets+i);
			init_color_bounds(clue->sets+i, BOUNDS_CUR, clue->sets[i].color_bounds_min[depth+BOUNDS_BCK], clue->sets[i].color_bounds_max[depth+BOUNDS_BCK]);
			init_empty_bounds(clue->sets+i, BOUNDS_CUR, clue->sets[i].empty_bounds_min[depth+BOUNDS_BCK], clue->sets[i].empty_bounds_max[depth+BOUNDS_BCK]);
		}
		clear_negative_empty_cache(depth, clue->sets+i);
	}
	return r;
}

int reallocate_bounds(set_t *set, int bounds_size) {
	int *empty_bounds_min, *empty_bounds_max, *color_bounds_min, *color_bounds_max;
	empty_bounds_min = realloc(set->empty_bounds_min, sizeof(int)*(size_t)bounds_size);
	if (!empty_bounds_min) {
		fprintf(stderr, "Could not reallocate memory for set->empty_bounds_min\n");
		fflush(stderr);
		return 0;
	}
	set->empty_bounds_min = empty_bounds_min;
	empty_bounds_max = realloc(set->empty_bounds_max, sizeof(int)*(size_t)bounds_size);
	if (!empty_bounds_max) {
		fprintf(stderr, "Could not allocate memory for set->empty_bounds_max\n");
		fflush(stderr);
		return 0;
	}
	set->empty_bounds_max = empty_bounds_max;
	color_bounds_min = realloc(set->color_bounds_min, sizeof(int)*(size_t)bounds_size);
	if (!color_bounds_min) {
		fprintf(stderr, "Could not allocate memory for set->color_bounds_min\n");
		fflush(stderr);
		return 0;
	}
	set->color_bounds_min = color_bounds_min;
	color_bounds_max = realloc(set->color_bounds_max, sizeof(int)*(size_t)bounds_size);
	if (!color_bounds_max) {
		fprintf(stderr, "Could not allocate memory for set->color_bounds_max\n");
		fflush(stderr);
		return 0;
	}
	set->color_bounds_max = color_bounds_max;
	return 1;
}

int compare_clues(const void *a, const void *b) {
	clue_t *clue_a = *(clue_t * const *)a, *clue_b = *(clue_t * const *)b;
	if (clue_a->sweeped != clue_b->sweeped) {
		return clue_b->sweeped-clue_a->sweeped;
	}
	return clue_a->pos-clue_b->pos;
}

void clear_positive_empty_cache(set_t *set) {
	int i;
	for (i = 0; i < set->empty_cache_size; i++) {
		if (set->empty_cache[i] > CACHE_EMPTY) {
			set->empty_cache[i] = CACHE_EMPTY;
		}
	}
}

void clear_positive_color_cache(set_t *set) {
	int i;
	for (i = set->color_bounds_min[BOUNDS_SWP]; i <= set->color_bounds_max[BOUNDS_SWP]; i++) {
		if (set->color_cache[i-set->color_bounds_min[BOUNDS_BCK]] > CACHE_EMPTY) {
			set->color_cache[i-set->color_bounds_min[BOUNDS_BCK]] = CACHE_EMPTY;
		}
	}
}

void clear_positive_cross_cache(set_t *set) {
	int i;
	for (i = set->color_bounds_min[BOUNDS_SWP]; i < set->len+set->color_bounds_max[BOUNDS_SWP]; i++) {
		if (set->cross_cache[i-set->color_bounds_min[BOUNDS_BCK]] > CACHE_EMPTY) {
			set->cross_cache[i-set->color_bounds_min[BOUNDS_BCK]] = CACHE_EMPTY;
		}
	}
}

void reset_clue_bounds(clue_t *clue, int bound_min) {
	set_t *set;
	for (set = clue->sets_header->next; set != clue->sets_header; set = set->next) {
		init_empty_bounds(set, BOUNDS_CUR, bound_min, -1);
		init_color_bounds(set, BOUNDS_CUR, bound_min, -1);
	}
}

int sweep_column(int depth, clue_t *clue, set_t *set, int pos) {
	int last_ok, r_sum, i;
	if (set->empty_cache[pos-set->empty_bounds_min[BOUNDS_BCK]] != CACHE_EMPTY) {
		if (set->empty_cache[pos-set->empty_bounds_min[BOUNDS_BCK]] > CACHE_EMPTY) {
			return set->empty_cache[pos-set->empty_bounds_min[BOUNDS_BCK]];
		}
		else {
			return 0;
		}
	}
	last_ok = pos;
	for (i = pos; i < set->color_bounds_min[BOUNDS_SWP] && (cells[i*columns_n+clue->pos].color == COLOR_EMPTY || cells[i*columns_n+clue->pos].color == COLOR_UNKNOWN); i++) {
		change_cell(cells+i*columns_n+clue->pos, COLOR_UNKNOWN, CLUE_POS_UNKNOWN, COLOR_EMPTY, clue->pos);
	}
	if (i >= set->color_bounds_min[BOUNDS_SWP]) {
		if (set == clue->sets_header) {
			last_ok = i;
			r_sum = 1;
		}
		else {
			int len;
			r_sum = sweep_column_empty_then_set(depth, clue, set, i, &len);
			if (r_sum > 0) {
				last_ok = i;
			}
			for (i++; i <= set->color_bounds_max[BOUNDS_SWP] && (cells[(i-1)*columns_n+clue->pos].color == COLOR_EMPTY || cells[(i-1)*columns_n+clue->pos].color == COLOR_UNKNOWN) && len >= 0; i++) {
				int r;
				if (len > 0 && len < set->len) {
					int j = i;
					if (i+len <= set->color_bounds_max[BOUNDS_SWP]) {
						i += len;
					}
					else {
						i = set->color_bounds_max[BOUNDS_SWP];
					}
					while (j < i) {
						change_cell(cells+(j-1)*columns_n+clue->pos, COLOR_UNKNOWN, CLUE_POS_UNKNOWN, COLOR_EMPTY, clue->pos);
						j++;
					}
				}
				change_cell(cells+(i-1)*columns_n+clue->pos, COLOR_UNKNOWN, CLUE_POS_UNKNOWN, COLOR_EMPTY, clue->pos);
				r = sweep_column_empty_then_set(depth, clue, set, i, &len);
				if (r > 0) {
					last_ok = i;
					r_sum = sum_with_limit(r_sum, r);
				}
			}
		}
	}
	else {
		r_sum = 0;
	}
	for (i--; i >= last_ok; i--) {
		change_cell(cells+i*columns_n+clue->pos, COLOR_EMPTY, clue->pos, COLOR_UNKNOWN, CLUE_POS_UNKNOWN);
	}
	for (; i >= pos; i--) {
		if (change_cell(cells+i*columns_n+clue->pos, COLOR_EMPTY, clue->pos, COLOR_UNKNOWN, CLUE_POS_UNKNOWN)) {
			check_column_cell(clue, cells+i*columns_n+clue->pos, COLOR_EMPTY);
		}
	}
	if (r_sum > 0) {
		if (pos < set->empty_bounds_min[BOUNDS_CUR]) {
			set->empty_bounds_min[BOUNDS_CUR] = pos;
		}
		if (pos > set->empty_bounds_max[BOUNDS_CUR]) {
			set->empty_bounds_max[BOUNDS_CUR] = pos;
		}
		set->empty_cache[pos-set->empty_bounds_min[BOUNDS_BCK]] = r_sum;
	}
	else {
		set->empty_cache[pos-set->empty_bounds_min[BOUNDS_BCK]] = -depth*2-1;
	}
	return r_sum;
}

int sweep_column_empty_then_set(int depth, clue_t *clue, set_t *set, int pos, int *len) {
	int r;
	if (set->color_cache[pos-set->color_bounds_min[BOUNDS_BCK]] != CACHE_EMPTY) {
		*len = 0;
		if (set->color_cache[pos-set->color_bounds_min[BOUNDS_BCK]] > CACHE_EMPTY) {
			return set->color_cache[pos-set->color_bounds_min[BOUNDS_BCK]];
		}
		else {
			return 0;
		}
	}
	if (set->empty_before) {
		if (cells[pos*columns_n+clue->pos].color == COLOR_EMPTY || cells[pos*columns_n+clue->pos].color == COLOR_UNKNOWN) {
			change_cell(cells+pos*columns_n+clue->pos, COLOR_UNKNOWN, CLUE_POS_UNKNOWN, COLOR_EMPTY, clue->pos);
			r = sweep_column_set(depth, clue, set, pos+1, len);
			if (change_cell(cells+pos*columns_n+clue->pos, COLOR_EMPTY, clue->pos, COLOR_UNKNOWN, CLUE_POS_UNKNOWN) && r > 0) {
				check_column_cell(clue, cells+pos*columns_n+clue->pos, COLOR_EMPTY);
			}
		}
		else {
			*len = 0;
			r = 0;
		}
	}
	else {
		r = sweep_column_set(depth, clue, set, pos, len);
	}
	if (r > 0) {
		if (pos < set->color_bounds_min[BOUNDS_CUR]) {
			set->color_bounds_min[BOUNDS_CUR] = pos;
		}
		if (pos > set->color_bounds_max[BOUNDS_CUR]) {
			set->color_bounds_max[BOUNDS_CUR] = pos;
		}
		set->color_cache[pos-set->color_bounds_min[BOUNDS_BCK]] = r;
	}
	else {
		set->color_cache[pos-set->color_bounds_min[BOUNDS_BCK]] = -depth*2-1;
	}
	return r;
}

int sweep_column_set(int depth, clue_t *clue, set_t *set, int pos, int *len) {
	int colored_cells_n = 0, r, i;
	for (i = pos; i < set->len+pos && ((cells[i*columns_n+clue->pos].color == COLOR_UNKNOWN && match_crossed_clue(depth, clues+width+i, clue->pos, set, i) > 0) || cells[i*columns_n+clue->pos].color == set->color); i++) {
		if (!change_cell(cells+i*columns_n+clue->pos, COLOR_UNKNOWN, CLUE_POS_UNKNOWN, set->color, clue->pos)) {
			colored_cells_n++;
		}
	}
	*len = i-pos;
	if (i == set->len+pos) {
		r = sweep_column(depth, clue, set+1, i);
	}
	else {
		if (colored_cells_n > 0) {
			*len = -*len;
		}
		r = 0;
	}
	if (r > 0) {
		for (i--; i >= pos; i--) {
			if (change_cell(cells+i*columns_n+clue->pos, set->color, clue->pos, COLOR_UNKNOWN, CLUE_POS_UNKNOWN)) {
				check_column_cell(clue, cells+i*columns_n+clue->pos, set->color);
			}
		}
	}
	else {
		for (i--; i >= pos; i--) {
			change_cell(cells+i*columns_n+clue->pos, set->color, clue->pos, COLOR_UNKNOWN, CLUE_POS_UNKNOWN);
		}
	}
	return r;
}

void check_column_cell(clue_t *clue, cell_t *cell, int color) {
	if (cells[cell->row*columns_n+width].color == COLOR_UNKNOWN) {
		init_cell(cells+cell->row*columns_n+width, color, clue->pos);
	}
	else if (cells[cell->row*columns_n+width].color != COLOR_SEVERAL) {
		if (cells[cell->row*columns_n+width].color != color) {
			init_cell(cells+cell->row*columns_n+width, COLOR_SEVERAL, clues_n);
		}
	}
}

int sweep_row(int depth, clue_t *clue, set_t *set, int pos) {
	int last_ok, r_sum, i;
	if (set->empty_cache[pos-set->empty_bounds_min[BOUNDS_BCK]] != CACHE_EMPTY) {
		if (set->empty_cache[pos-set->empty_bounds_min[BOUNDS_BCK]] > CACHE_EMPTY) {
			return set->empty_cache[pos-set->empty_bounds_min[BOUNDS_BCK]];
		}
		else {
			return 0;
		}
	}
	last_ok = pos;
	for (i = pos; i < set->color_bounds_min[BOUNDS_SWP] && (cells[(clue->pos-width)*columns_n+i].color == COLOR_EMPTY || cells[(clue->pos-width)*columns_n+i].color == COLOR_UNKNOWN); i++) {
		change_cell(cells+(clue->pos-width)*columns_n+i, COLOR_UNKNOWN, CLUE_POS_UNKNOWN, COLOR_EMPTY, clue->pos);
	}
	if (i >= set->color_bounds_min[BOUNDS_SWP]) {
		if (set == clue->sets_header) {
			last_ok = i;
			r_sum = 1;
		}
		else {
			int len;
			r_sum = sweep_row_empty_then_set(depth, clue, set, i, &len);
			if (r_sum > 0) {
				last_ok = i;
			}
			for (i++; i <= set->color_bounds_max[BOUNDS_SWP] && (cells[(clue->pos-width)*columns_n+i-1].color == COLOR_EMPTY || cells[(clue->pos-width)*columns_n+i-1].color == COLOR_UNKNOWN) && len >= 0; i++) {
				int r;
				if (len > 0 && len < set->len) {
					int j = i;
					if (i+len <= set->color_bounds_max[BOUNDS_SWP]) {
						i += len;
					}
					else {
						i = set->color_bounds_max[BOUNDS_SWP];
					}
					while (j < i) {
						change_cell(cells+(clue->pos-width)*columns_n+j-1, COLOR_UNKNOWN, CLUE_POS_UNKNOWN, COLOR_EMPTY, clue->pos);
						j++;
					}
				}
				change_cell(cells+(clue->pos-width)*columns_n+i-1, COLOR_UNKNOWN, CLUE_POS_UNKNOWN, COLOR_EMPTY, clue->pos);
				r = sweep_row_empty_then_set(depth, clue, set, i, &len);
				if (r > 0) {
					last_ok = i;
					r_sum = sum_with_limit(r_sum, r);
				}
			}
		}
	}
	else {
		r_sum = 0;
	}
	for (i--; i >= last_ok; i--) {
		change_cell(cells+(clue->pos-width)*columns_n+i, COLOR_EMPTY, clue->pos, COLOR_UNKNOWN, CLUE_POS_UNKNOWN);
	}
	for (; i >= pos; i--) {
		if (change_cell(cells+(clue->pos-width)*columns_n+i, COLOR_EMPTY, clue->pos, COLOR_UNKNOWN, CLUE_POS_UNKNOWN)) {
			check_row_cell(clue, cells+(clue->pos-width)*columns_n+i, COLOR_EMPTY);
		}
	}
	if (r_sum > 0) {
		if (pos < set->empty_bounds_min[BOUNDS_CUR]) {
			set->empty_bounds_min[BOUNDS_CUR] = pos;
		}
		if (pos > set->empty_bounds_max[BOUNDS_CUR]) {
			set->empty_bounds_max[BOUNDS_CUR] = pos;
		}
		set->empty_cache[pos-set->empty_bounds_min[BOUNDS_BCK]] = r_sum;
	}
	else {
		set->empty_cache[pos-set->empty_bounds_min[BOUNDS_BCK]] = -depth*2-1;
	}
	return r_sum;
}

int sweep_row_empty_then_set(int depth, clue_t *clue, set_t *set, int pos, int *len) {
	int r;
	if (set->color_cache[pos-set->color_bounds_min[BOUNDS_BCK]] != CACHE_EMPTY) {
		*len = 0;
		if (set->color_cache[pos-set->color_bounds_min[BOUNDS_BCK]] > CACHE_EMPTY) {
			return set->color_cache[pos-set->color_bounds_min[BOUNDS_BCK]];
		}
		else {
			return 0;
		}
	}
	if (set->empty_before) {
		if (cells[(clue->pos-width)*columns_n+pos].color == COLOR_EMPTY || cells[(clue->pos-width)*columns_n+pos].color == COLOR_UNKNOWN) {
			change_cell(cells+(clue->pos-width)*columns_n+pos, COLOR_UNKNOWN, CLUE_POS_UNKNOWN, COLOR_EMPTY, clue->pos);
			r = sweep_row_set(depth, clue, set, pos+1, len);
			if (change_cell(cells+(clue->pos-width)*columns_n+pos, COLOR_EMPTY, clue->pos, COLOR_UNKNOWN, CLUE_POS_UNKNOWN) && r > 0) {
				check_row_cell(clue, cells+(clue->pos-width)*columns_n+pos, COLOR_EMPTY);
			}
		}
		else {
			*len = 0;
			r = 0;
		}
	}
	else {
		r = sweep_row_set(depth, clue, set, pos, len);
	}
	if (r > 0) {
		if (pos < set->color_bounds_min[BOUNDS_CUR]) {
			set->color_bounds_min[BOUNDS_CUR] = pos;
		}
		if (pos > set->color_bounds_max[BOUNDS_CUR]) {
			set->color_bounds_max[BOUNDS_CUR] = pos;
		}
		set->color_cache[pos-set->color_bounds_min[BOUNDS_BCK]] = r;
	}
	else {
		set->color_cache[pos-set->color_bounds_min[BOUNDS_BCK]] = -depth*2-1;
	}
	return r;
}

int sweep_row_set(int depth, clue_t *clue, set_t *set, int pos, int *len) {
	int colored_cells_n = 0, r, i;
	for (i = pos; i < set->len+pos && ((cells[(clue->pos-width)*columns_n+i].color == COLOR_UNKNOWN && match_crossed_clue(depth, clues+i, clue->pos-width, set, i) > 0) || cells[(clue->pos-width)*columns_n+i].color == set->color); i++) {
		if (!change_cell(cells+(clue->pos-width)*columns_n+i, COLOR_UNKNOWN, CLUE_POS_UNKNOWN, set->color, clue->pos)) {
			colored_cells_n++;
		}
	}
	*len = i-pos;
	if (i == set->len+pos) {
		r = sweep_row(depth, clue, set+1, i);
	}
	else {
		if (colored_cells_n > 0) {
			*len = -*len;
		}
		r = 0;
	}
	if (r > 0) {
		for (i--; i >= pos; i--) {
			if (change_cell(cells+(clue->pos-width)*columns_n+i, set->color, clue->pos, COLOR_UNKNOWN, CLUE_POS_UNKNOWN)) {
				check_row_cell(clue, cells+(clue->pos-width)*columns_n+i, set->color);
			}
		}
	}
	else {
		for (i--; i >= pos; i--) {
			change_cell(cells+(clue->pos-width)*columns_n+i, set->color, clue->pos, COLOR_UNKNOWN, CLUE_POS_UNKNOWN);
		}
	}
	return r;
}

void check_row_cell(clue_t *clue, cell_t *cell, int color) {
	if (cells[height*columns_n+cell->column].color == COLOR_UNKNOWN) {
		init_cell(cells+height*columns_n+cell->column, color, clue->pos);
	}
	else if (cells[height*columns_n+cell->column].color != COLOR_SEVERAL) {
		if (cells[height*columns_n+cell->column].color != color) {
			init_cell(cells+height*columns_n+cell->column, COLOR_SEVERAL, clues_n);
		}
	}
}

int sum_with_limit(int a, int b) {
	if (a <= INT_MAX-b) {
		return a+b;
	}
	return INT_MAX;
}

int match_crossed_clue(int depth, clue_t *crossed, int clue_pos, set_t *set, int cell_pos) {
	int r, i;
	if (set->cross_cache[cell_pos-set->empty_before-set->color_bounds_min[BOUNDS_BCK]] != CACHE_EMPTY) {
		return set->cross_cache[cell_pos-set->empty_before-set->color_bounds_min[BOUNDS_BCK]] > CACHE_EMPTY;
	}
	r = 0;
	for (i = 0; i < crossed->sets_n && crossed->sets[i].empty_before+crossed->sets[i].color_bounds_min[BOUNDS_CUR] <= clue_pos; i++) {
		if (crossed->sets[i].len+crossed->sets[i].empty_before+crossed->sets[i].color_bounds_max[BOUNDS_CUR] > clue_pos && crossed->sets[i].color == set->color) {
			r++;
		}
	}
	if (r > 0) {
		set->cross_cache[cell_pos-set->empty_before-set->color_bounds_min[BOUNDS_BCK]] = r;
	}
	else {
		set->cross_cache[cell_pos-set->empty_before-set->color_bounds_min[BOUNDS_BCK]] = -depth*2-1;
	}
	return r;
}

void cover_cell(cell_t *cell) {
	cell->column_last->column_next = cell->column_next;
	cell->column_next->column_last = cell->column_last;
	cell->row_last->row_next = cell->row_next;
	cell->row_next->row_last = cell->row_last;
}

void cover_clue(clue_t *clue) {
	clue->last->next = clue->next;
	clue->next->last = clue->last;
}

void cover_set(set_t *set) {
	set->last->next = set->next;
	set->next->last = set->last;
}

int compare_sets(const void *a, const void *b) {
	set_t *set_a = *(set_t * const *)a, *set_b = *(set_t * const *)b;
	if (set_a->options_n != set_b->options_n) {
		return set_a->options_n-set_b->options_n;
	}
	if (set_a < set_b) {
		return -1;
	}
	return 1;
}

void evaluate_set(int depth, int locked_sets_sum, int locked_clues_sum, int locked_cells_sum, int set_options_n_min, set_t *set, int *set_options_n, int *set_solutions_n, int *set_vals_sum) {
	int i;
	*set_options_n = 0;
	*set_solutions_n = 0;
	*set_vals_sum = 0;
	for (i = 0; i < set->options_n; i++) {
		set->color_cache[set->options[i].cell_pos-set->color_bounds_min[BOUNDS_BCK]] = -depth*2-2;
	}
	for (i = 0; i < set->options_n && *set_options_n <= set_options_n_min; i++) {
		set->color_cache[set->options[i].cell_pos-set->color_bounds_min[BOUNDS_BCK]] = CACHE_EMPTY;
		set->options[i].val = nonogram(depth, locked_sets_sum, locked_clues_sum, locked_cells_sum, 0);
		set->color_cache[set->options[i].cell_pos-set->color_bounds_min[BOUNDS_BCK]] = -depth*2-2;
		if (set->options[i].val >= 0) {
			*set_options_n = *set_options_n+1;
			if (locked_cells_sum+set->options[i].val == grid_size) {
				*set_solutions_n = *set_solutions_n+1;
			}
			else {
				*set_vals_sum += set->options[i].val;
			}
		}
	}
	for (i = 0; i < set->options_n; i++) {
		set->color_cache[set->options[i].cell_pos-set->color_bounds_min[BOUNDS_BCK]] = CACHE_EMPTY;
	}
}

void uncover_set(set_t *set) {
	set->next->last = set;
	set->last->next = set;
}

void uncover_clue(clue_t *clue) {
	clue->next->last = clue;
	clue->last->next = clue;
}

void uncover_cell(cell_t *cell) {
	cell->row_next->row_last = cell;
	cell->row_last->row_next = cell;
	cell->column_next->column_last = cell;
	cell->column_last->column_next = cell;
}

void clear_negative_empty_cache(int depth, set_t *set) {
	int i;
	for (i = 0; i < set->empty_cache_size; i++) {
		if (set->empty_cache[i] == -depth*2-1) {
			set->empty_cache[i] = CACHE_EMPTY;
		}
	}
}

void clear_negative_color_cache(int depth, set_t *set) {
	int i;
	for (i = set->color_bounds_min[depth+BOUNDS_BCK]; i <= set->color_bounds_max[depth+BOUNDS_BCK]; i++) {
		if (set->color_cache[i-set->color_bounds_min[BOUNDS_BCK]] == -depth*2-1) {
			set->color_cache[i-set->color_bounds_min[BOUNDS_BCK]] = CACHE_EMPTY;
		}
	}
}

void clear_negative_cross_cache(int depth, set_t *set) {
	int i;
	for (i = set->color_bounds_min[depth+BOUNDS_BCK]; i < set->len+set->color_bounds_max[depth+BOUNDS_BCK]; i++) {
		if (set->cross_cache[i-set->color_bounds_min[BOUNDS_BCK]] == -depth*2-1) {
			set->cross_cache[i-set->color_bounds_min[BOUNDS_BCK]] = CACHE_EMPTY;
		}
	}
}

void init_empty_bounds(set_t *set, int pos, int bound_min, int bound_max) {
	set->empty_bounds_min[pos] = bound_min;
	set->empty_bounds_max[pos] = bound_max;
}

void init_color_bounds(set_t *set, int pos, int bound_min, int bound_max) {
	set->color_bounds_min[pos] = bound_min;
	set->color_bounds_max[pos] = bound_max;
}

int change_cell(cell_t *cell, int from_color, int from_clue_pos, int to_color, int to_clue_pos) {
	if (cell->color == from_color && cell->clue_pos == from_clue_pos) {
		init_cell(cell, to_color, to_clue_pos);
		return 1;
	}
	return 0;
}

void init_cell(cell_t *cell, int color, int clue_pos) {
	cell->color = color;
	cell->clue_pos = clue_pos;
}

void free_clues(int clues_n_max) {
	int i;
	for (i = 0; i < clues_n_max; i++) {
		free_clue(clues+i);
	}
	free(clues);
}

void free_clue(clue_t *clue) {
	int i;
	for (i = 0; i < clue->sets_n; i++) {
		free_set(clue->sets+i);
	}
	free(clue->sets);
}

void free_set(set_t *set) {
	if (set->options) {
		free(set->options);
		set->options = NULL;
	}
	if (set->cross_cache) {
		free(set->cross_cache);
		set->cross_cache = NULL;
	}
	if (set->color_cache) {
		free(set->color_cache);
		set->color_cache = NULL;
	}
	if (set->empty_cache) {
		free(set->empty_cache);
		set->empty_cache = NULL;
	}
	if (set->color_bounds_max) {
		free(set->color_bounds_max);
		set->color_bounds_max = NULL;
	}
	if (set->color_bounds_min) {
		free(set->color_bounds_min);
		set->color_bounds_min = NULL;
	}
	if (set->empty_bounds_max) {
		free(set->empty_bounds_max);
		set->empty_bounds_max = NULL;
	}
	if (set->empty_bounds_min) {
		free(set->empty_bounds_min);
		set->empty_bounds_min = NULL;
	}
}
