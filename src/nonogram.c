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
#define COLOR_SEVERAL '!'
#define COLOR_EMPTY ' '
#define COLOR_BLACK '*'
#define COLOR_POS_UNKNOWN 0
#define COLOR_POS_SEVERAL 1
#define COLOR_POS_EMPTY 2
#define COLOR_POS_BLACK 3
#define DEPTH_CUR 0
#define DEPTH_BCK 1
#define DEPTHS_SIZE 2
#define CACHE_UNKNOWN 0
#define CACHE_CONFIRMED 1

typedef struct {
	int pos;
	int r;
	int changes_sum;
}
option_t;

typedef struct set_s set_t;
typedef struct clue_s clue_t;
typedef struct cell_s cell_t;

struct set_s {
	int len;
	int color_pos;
	clue_t *clue;
	int empty_before;
	int *empty_bounds_min;
	int *color_bounds_min;
	int *color_bounds_max;
	int *empty_cache;
	int *color_cache;
	int options_n;
	option_t *options;
	int solutions_n;
	int changes_sum;
	int skipped;
	int others_n;
	set_t *last;
	set_t *next;
};

struct clue_s {
	int pos;
	int sets_n;
	set_t *sets;
	set_t *sets_header;
	int depths_size;
	int priority;
	cell_t *cells_header;
	clue_t *last;
	clue_t *next;
};

struct cell_s {
	clue_t *column_clue;
	clue_t *row_clue;
	int color_pos;
	int *color_cache;
	cell_t *column_last;
	cell_t *column_next;
	cell_t *row_last;
	cell_t *row_next;
};

static int read_clue(clue_t *, int, int);
static void init_set_fields(set_t *, int, int, clue_t *);
static int init_set_tables(set_t *, int, int, int, int);
static void link_set(set_t *, set_t *, set_t *);
static void link_clue(clue_t *, clue_t *, clue_t *);
static int init_cell(cell_t *, clue_t *, clue_t *);
static void link_column_clue(clue_t *, cell_t *);
static void link_column_cell(cell_t *, cell_t *, cell_t *);
static void link_row_clue(clue_t *, cell_t *);
static void link_row_cell(cell_t *, cell_t *, cell_t *);
static void nonogram(clue_t *, option_t *);
static void init_option(option_t *, int, int);
static int reallocate_bounds(set_t *, int);
static int compare_priorities(const void *, const void *);
static int sweep_clue(set_t *, int, cell_t *);
static int sweep_set(set_t *, int, cell_t *, cell_t **, int *);
static int check_cell_colors(cell_t *);
static void update_empty_bound_and_cache(int *, const int *, int *);
static void update_color_bounds_and_cache(int *, int *, int *);
static void clear_set_positive_cache(const int *, const int *, int *);
static int compare_sets(const void *, const void *);
static int evaluate_set(set_t *);
static void evaluate_option(set_t *, option_t *);
static set_t *init_set_min(set_t *);
static int compare_evaluations(const set_t *, const set_t *);
static int compare_options(const void *, const void *);
static void uncover_set(set_t *);
static void uncover_clue(clue_t *);
static void uncover_cell(cell_t *);
static void clear_set_negative_cache(const int *, const int *, int *);
static void init_empty_bounds(set_t *, int, int);
static void init_color_bounds(set_t *, int, int, int);
static unsigned inc_with_limit(unsigned);
static int sum_with_limit(int, int);
static void *alloc_mem(const char *, size_t, int);
static void *realloc_mem(const char *, void *, size_t, int);
static void free_data(int, int);
static void free_cell(cell_t *);
static void free_clue(clue_t *, int);
static void free_set(set_t *);
static void free_ints(int **);

static int width, height, colors_n, *colors, clues_n, grid_size, depth, completed_depth, negative_cache, offset;
static unsigned time_zero, all_nodes_n, run_nodes_n, failures_n, locked_cells_n, locked_clues_n, locked_sets_n;
static unsigned long verbose, solutions_max, solutions_n;
static set_t **sorted_sets, **locked_sets;
static clue_t *clues, *clues_header, **sorted_clues, **locked_clues, *current_clue;
static cell_t *cells, **locked_cells;

int main(int argc, char *argv[]) {
	char *end;
	int sets_n, i;
	if (argc < 2 || argc > 3) {
		fprintf(stderr, "Usage: %s <verbose flag> [<maximum number of solutions>]\n", argv[0]);
		fflush(stderr);
		return EXIT_FAILURE;
	}
	verbose = strtoul(argv[1], &end, 10);
	if (*end) {
		fputs("Invalid verbose flag\n", stderr);
		fflush(stderr);
		return EXIT_FAILURE;
	}
	if (argc == 2) {
		solutions_max = ULONG_MAX;
	}
	else {
		solutions_max = strtoul(argv[2], &end, 10);
		if (*end || solutions_max < 1UL) {
			fputs("Invalid maximum number of solutions\n", stderr);
			fflush(stderr);
			return EXIT_FAILURE;
		}
	}
	if (scanf("%d%d", &width, &height) != 2 || width < 1 || height < 1) {
		fputs("Invalid grid attributes\n", stderr);
		fflush(stderr);
		return EXIT_FAILURE;
	}
	getchar();
	colors_n = COLOR_POS_BLACK;
	colors = alloc_mem("colors", sizeof(int), colors_n);
	if (!colors) {
		return EXIT_FAILURE;
	}
	colors[COLOR_POS_UNKNOWN] = COLOR_UNKNOWN;
	colors[COLOR_POS_SEVERAL] = COLOR_SEVERAL;
	colors[COLOR_POS_EMPTY] = COLOR_EMPTY;
	clues = NULL;
	cells = NULL;
	sorted_sets = NULL;
	locked_sets = NULL;
	sorted_clues = NULL;
	locked_clues = NULL;
	locked_cells = NULL;
	clues_n = width+height;
	clues = alloc_mem("clues", sizeof(clue_t), clues_n+1);
	if (!clues) {
		free_data(0, 0);
		return EXIT_FAILURE;
	}
	for (i = 0; i < width-1; ++i) {
		if (!read_clue(clues+i, i, SEPARATOR_CLUES)) {
			free_data(0, i);
			return EXIT_FAILURE;
		}
	}
	if (!read_clue(clues+i, i, SEPARATOR_LINES)) {
		free_data(0, i);
		return EXIT_FAILURE;
	}
	for (++i; i < clues_n-1; ++i) {
		if (!read_clue(clues+i, i, SEPARATOR_CLUES)) {
			free_data(0, i);
			return EXIT_FAILURE;
		}
	}
	if (!read_clue(clues+i, i, SEPARATOR_LINES)) {
		free_data(0, i);
		return EXIT_FAILURE;
	}
	clues_header = clues+clues_n;
	clues_header->pos = clues_n;
	link_clue(clues, clues_header, clues+1);
	for (i = 1; i < clues_n; ++i) {
		link_clue(clues+i, clues+i-1, clues+i+1);
	}
	link_clue(clues_header, clues_header-1, clues);
	grid_size = width*height;
	cells = alloc_mem("cells", sizeof(cell_t), grid_size+clues_n);
	if (!cells) {
		free_data(0, clues_n);
		return EXIT_FAILURE;
	}
	for (i = width; i--; ) {
		int j;
		for (j = height; j--; ) {
			if (!init_cell(cells+j*width+i, clues+i, clues+width+j)) {
				free_data(j*width+i, clues_n);
				return EXIT_FAILURE;
			}
		}
	}
	for (i = width; i--; ) {
		link_column_clue(clues+i, cells+i);
	}
	for (i = height; i--; ) {
		link_row_clue(clues+width+i, cells+i*width);
	}
	sets_n = clues[0].sets_n;
	for (i = 1; i < clues_n; ++i) {
		sets_n += clues[i].sets_n;
	}
	if (sets_n) {
		sorted_sets = alloc_mem("sorted_sets", sizeof(set_t *), sets_n);
		if (!sorted_sets) {
			free_data(grid_size, clues_n);
			return EXIT_FAILURE;
		}
		locked_sets = alloc_mem("locked_sets", sizeof(set_t *), sets_n);
		if (!locked_sets) {
			free_data(grid_size, clues_n);
			return EXIT_FAILURE;
		}
	}
	sorted_clues = alloc_mem("sorted_clues", sizeof(clue_t *), clues_n);
	if (!sorted_clues) {
		free_data(grid_size, clues_n);
		return EXIT_FAILURE;
	}
	locked_clues = alloc_mem("locked_clues", sizeof(clue_t *), clues_n);
	if (!locked_clues) {
		free_data(grid_size, clues_n);
		return EXIT_FAILURE;
	}
	locked_cells = alloc_mem("locked_cells", sizeof(cell_t *), grid_size);
	if (!locked_cells) {
		free_data(grid_size, clues_n);
		return EXIT_FAILURE;
	}
	time_zero = (unsigned)time(NULL);
	all_nodes_n = 0U;
	run_nodes_n = 0U;
	failures_n = 0U;
	solutions_n = 0UL;
	depth = 0;
	completed_depth = 0;
	negative_cache = -1;
	locked_cells_n = 0U;
	locked_clues_n = 0U;
	locked_sets_n = 0U;
	nonogram(NULL, NULL);
	printf("\nTime %us\nAll nodes %u\nRun nodes %u\nFailures %u\nSolutions %lu\n", (unsigned)time(NULL)-time_zero, all_nodes_n, run_nodes_n, failures_n, solutions_n);
	fflush(stdout);
	free_data(grid_size, clues_n);
	return EXIT_SUCCESS;
}

static int read_clue(clue_t *clue, int pos, int separator) {
	int c, len_min;
	if (getchar() != DELIMITER_CLUE) {
		fputs("Double quote expected as clue start\n", stderr);
		fflush(stderr);
		return 0;
	}
	clue->pos = pos;
	clue->sets_n = 0;
	clue->sets = alloc_mem("clue->sets", sizeof(set_t), 1);
	if (!clue->sets) {
		return 0;
	}
	do {
		int len;
		if (scanf("%d", &len) != 1 || len < 0) {
			fputs("Invalid set length\n", stderr);
			fflush(stderr);
			free(clue->sets);
			return 0;
		}
		c = getchar();
		if (len) {
			int color, color_pos;
			if (c == SEPARATOR_COLOR) {
				color = getchar();
				if (!isalnum(color)) {
					fputs("Invalid set color\n", stderr);
					fflush(stderr);
					free(clue->sets);
					return 0;
				}
				c = getchar();
			}
			else {
				color = COLOR_BLACK;
			}
			for (color_pos = COLOR_POS_BLACK; color_pos < colors_n && colors[color_pos] != color; ++color_pos);
			if (color_pos == colors_n) {
				int *colors_tmp = realloc_mem("colors", colors, sizeof(int), colors_n+1);
				if (!colors_tmp) {
					free(clue->sets);
					return 0;
				}
				colors = colors_tmp;
				colors[colors_n++] = color;
			}
			set_t *sets = realloc_mem("clue->sets", clue->sets, sizeof(set_t), clue->sets_n+2);
			if (!sets) {
				free(clue->sets);
				return 0;
			}
			clue->sets = sets;
			init_set_fields(clue->sets+clue->sets_n, len, color_pos, clue);
			++clue->sets_n;
		}
		if (c != SEPARATOR_SETS && c != DELIMITER_CLUE) {
			fputs("Invalid set separator\n", stderr);
			fflush(stderr);
			free(clue->sets);
			return 0;
		}
	}
	while (c != DELIMITER_CLUE);
	clue->sets_header = clue->sets+clue->sets_n;
	init_set_fields(clue->sets_header, 0, COLOR_POS_UNKNOWN, clue);
	clue->depths_size = DEPTHS_SIZE;
	clue->sets[0].empty_before = 0;
	len_min = clue->sets[0].len;
	if (clue->sets_n) {
		int relaxation, i;
		for (i = 1; i < clue->sets_n; ++i) {
			if (clue->sets[i].color_pos == clue->sets[i-1].color_pos) {
				clue->sets[i].empty_before = 1;
				++len_min;
			}
			else {
				clue->sets[i].empty_before = 0;
			}
			len_min += clue->sets[i].len;
		}
		clue->sets[i].empty_before = 0;
		if (clue->pos < width) {
			if (len_min > height) {
				fputs("Incompatible clue\n", stderr);
				fflush(stderr);
				free(clue->sets);
				return 0;
			}
			relaxation = height-len_min;
		}
		else {
			if (len_min > width) {
				fputs("Incompatible clue\n", stderr);
				fflush(stderr);
				free(clue->sets);
				return 0;
			}
			relaxation = width-len_min;
		}
		clue->priority = len_min-relaxation;
		if (!init_set_tables(clue->sets, 0, relaxation, 0, relaxation)) {
			free(clue->sets);
			return 0;
		}
		link_set(clue->sets, clue->sets_header, clue->sets+1);
		len_min = clue->sets[0].len+clue->sets[0].empty_before;
		for (i = 1; i < clue->sets_n; ++i) {
			if (!init_set_tables(clue->sets+i, len_min, len_min+relaxation, len_min, len_min+relaxation)) {
				free_clue(clue, i);
				return 0;
			}
			link_set(clue->sets+i, clue->sets+i-1, clue->sets+i+1);
			len_min += clue->sets[i].len+clue->sets[i].empty_before;
		}
	}
	else {
		clue->priority = clue->pos < width ? height:width;
	}
	if (clue->pos < width) {
		if (!init_set_tables(clue->sets_header, len_min, height, height, height-1)) {
			free_clue(clue, clue->sets_n);
			return 0;
		}
		init_empty_bounds(clue->sets_header, DEPTH_BCK, len_min);
		init_color_bounds(clue->sets_header, DEPTH_BCK, height, height-1);
	}
	else {
		if (!init_set_tables(clue->sets_header, len_min, width, width, width-1)) {
			free_clue(clue, clue->sets_n);
			return 0;
		}
		init_empty_bounds(clue->sets_header, DEPTH_BCK, len_min);
		init_color_bounds(clue->sets_header, DEPTH_BCK, width, width-1);
	}
	clue->sets_n ? link_set(clue->sets_header, clue->sets_header-1, clue->sets):link_set(clue->sets_header, clue->sets_header, clue->sets_header);
	if (getchar() != separator) {
		fputs("Invalid clue separator\n", stderr);
		fflush(stderr);
		free_clue(clue, clue->sets_n+1);
		return 0;
	}
	return 1;
}

static void init_set_fields(set_t *set, int len, int color_pos, clue_t *clue) {
	set->len = len;
	set->color_pos = color_pos;
	set->clue = clue;
	set->empty_bounds_min = NULL;
	set->color_bounds_min = NULL;
	set->color_bounds_max = NULL;
	set->empty_cache = NULL;
	set->color_cache = NULL;
	set->options = NULL;
}

static int init_set_tables(set_t *set, int empty_bound_min, int empty_bound_max, int color_bound_min, int color_bound_max) {
	int cache_size, i;
	set->empty_bounds_min = alloc_mem("set->empty_bounds_min", sizeof(int), DEPTHS_SIZE);
	if (!set->empty_bounds_min) {
		return 0;
	}
	init_empty_bounds(set, DEPTH_CUR, empty_bound_min);
	set->color_bounds_min = alloc_mem("set->color_bounds_min", sizeof(int), DEPTHS_SIZE);
	if (!set->color_bounds_min) {
		free_set(set);
		return 0;
	}
	set->color_bounds_max = alloc_mem("set->color_bounds_max", sizeof(int), DEPTHS_SIZE);
	if (!set->color_bounds_max) {
		free_set(set);
		return 0;
	}
	init_color_bounds(set, DEPTH_CUR, color_bound_min, color_bound_max);
	cache_size = empty_bound_max-empty_bound_min+1;
	set->empty_cache = alloc_mem("set->empty_cache", sizeof(int), cache_size);
	if (!set->empty_cache) {
		free_set(set);
		return 0;
	}
	for (i = cache_size; i--; ) {
		set->empty_cache[i] = CACHE_UNKNOWN;
	}
	cache_size = color_bound_max-color_bound_min+1;
	if (cache_size) {
		set->color_cache = alloc_mem("set->color_cache", sizeof(int), cache_size);
		if (!set->color_cache) {
			free_set(set);
			return 0;
		}
		for (i = cache_size; i--; ) {
			set->color_cache[i] = CACHE_UNKNOWN;
		}
		set->options = alloc_mem("set->options", sizeof(option_t), cache_size);
		if (!set->options) {
			free_set(set);
			return 0;
		}
	}
	return 1;
}

static void link_set(set_t *set, set_t *last, set_t *next) {
	set->last = last;
	set->next = next;
}

static void link_clue(clue_t *clue, clue_t *last, clue_t *next) {
	clue->last = last;
	clue->next = next;
}

static int init_cell(cell_t *cell, clue_t *column_clue, clue_t *row_clue) {
	int i;
	cell->column_clue = column_clue;
	cell->row_clue = row_clue;
	cell->color_pos = COLOR_POS_UNKNOWN;
	cell->color_cache = alloc_mem("cell->color_cache", sizeof(int), colors_n-COLOR_POS_EMPTY);
	if (!cell->color_cache) {
		return 0;
	}
	for (i = COLOR_POS_EMPTY; i < colors_n; ++i) {
		cell->color_cache[i-COLOR_POS_EMPTY] = CACHE_UNKNOWN;
	}
	return 1;
}

static void link_column_clue(clue_t *clue, cell_t *cell_first) {
	clue->cells_header = cells+grid_size+clue->pos;
	if (height > 1) {
		int i;
		link_column_cell(cell_first, clue->cells_header, cell_first+width);
		for (i = width; i < grid_size-width; i += width) {
			link_column_cell(cell_first+i, cell_first+i-width, cell_first+i+width);
		}
		link_column_cell(cell_first+i, cell_first+i-width, clue->cells_header);
		link_column_cell(clue->cells_header, cell_first+i, cell_first);
	}
	else {
		link_column_cell(cell_first, clue->cells_header, clue->cells_header);
		link_column_cell(clue->cells_header, cell_first, cell_first);
	}
}

static void link_column_cell(cell_t *cell, cell_t *last, cell_t *next) {
	cell->column_last = last;
	cell->column_next = next;
}

static void link_row_clue(clue_t *clue, cell_t *cell_first) {
	clue->cells_header = cells+grid_size+clue->pos;
	if (width > 1) {
		int i;
		link_row_cell(cell_first, clue->cells_header, cell_first+1);
		for (i = 1; i < width-1; ++i) {
			link_row_cell(cell_first+i, cell_first+i-1, cell_first+i+1);
		}
		link_row_cell(cell_first+i, cell_first+i-1, clue->cells_header);
		link_row_cell(clue->cells_header, cell_first+i, cell_first);
	}
	else {
		link_row_cell(cell_first, clue->cells_header, clue->cells_header);
		link_row_cell(clue->cells_header, cell_first, cell_first);
	}
}

static void link_row_cell(cell_t *cell, cell_t *last, cell_t *next) {
	cell->row_last = last;
	cell->row_next = next;
}

static void nonogram(clue_t *clue_first, option_t *evaluated) {
	int changes_sum, sorted_clues_n, changes_n, clue_options_min;
	unsigned locked_cells_n_bak, locked_clues_n_bak;
	clue_t *clue;
	all_nodes_n = inc_with_limit(all_nodes_n);
	if (!evaluated) {
		run_nodes_n = inc_with_limit(run_nodes_n);
	}
	if (solutions_n == solutions_max) {
		if (evaluated) {
			init_option(evaluated, -1, 0);
		}
		return;
	}
	locked_cells_n_bak = locked_cells_n;
	locked_clues_n_bak = locked_clues_n;
	changes_sum = 0;
	for (clue = clues_header->next; clue != clues_header; clue = clue->next) {
		int i;
		if (clue->depths_size < depth+DEPTHS_SIZE) {
			for (i = clue->sets_n+1; i--; ) {
				if (!reallocate_bounds(clue->sets+i, depth+DEPTHS_SIZE)) {
					if (evaluated) {
						init_option(evaluated, -1, 0);
					}
					return;
				}
			}
			clue->depths_size = depth+DEPTHS_SIZE;
		}
		for (i = clue->sets_n+1; i--; ) {
			init_empty_bounds(clue->sets+i, depth+DEPTH_BCK, clue->sets[i].empty_bounds_min[DEPTH_CUR]);
			init_color_bounds(clue->sets+i, depth+DEPTH_BCK, clue->sets[i].color_bounds_min[DEPTH_CUR], clue->sets[i].color_bounds_max[DEPTH_CUR]);
		}
	}
	if (clue_first) {
		sorted_clues[0] = clue_first;
		sorted_clues_n = 1;
	}
	else {
		sorted_clues_n = 0;
		for (clue = clues_header->next; clue != clues_header; clue = clue->next) {
			sorted_clues[sorted_clues_n++] = clue;
		}
		qsort(sorted_clues, (size_t)sorted_clues_n, sizeof(clue_t *), compare_priorities);
	}
	do {
		int i;
		changes_n = 0;
		clue_options_min = INT_MAX;
		for (clue = clues_header->next; clue != clues_header; clue = clue->next) {
			clue->priority = 0;
		}
		for (i = 0; i < sorted_clues_n; i++) {
			int clue_options_n, j;
			cell_t *cell;
			current_clue = sorted_clues[i];
			if (current_clue->pos < width) {
				offset = width;
				clue_options_n = sweep_clue(current_clue->sets, 0, cells+current_clue->pos);
			}
			else {
				offset = 1;
				clue_options_n = sweep_clue(current_clue->sets, 0, cells+(current_clue->pos-width)*width);
			}
			if (clue_options_n < clue_options_min) {
				clue_options_min = clue_options_n;
				if (clue_options_min == negative_cache) {
					break;
				}
			}
			if (current_clue->pos < width) {
				for (cell = current_clue->cells_header->column_next; cell != current_clue->cells_header; cell = cell->column_next) {
					changes_n += check_cell_colors(cell);
				}
			}
			else {
				for (cell = current_clue->cells_header->row_next; cell != current_clue->cells_header; cell = cell->row_next) {
					changes_n += check_cell_colors(cell);
				}
			}
			for (j = 0; j < current_clue->sets_n; ++j) {
				update_empty_bound_and_cache(current_clue->sets[j].empty_bounds_min, current_clue->sets[j].color_bounds_max, current_clue->sets[j].empty_cache);
				update_color_bounds_and_cache(current_clue->sets[j].color_bounds_min, current_clue->sets[j].color_bounds_max, current_clue->sets[j].color_cache);
			}
			update_empty_bound_and_cache(current_clue->sets[j].empty_bounds_min, current_clue->sets[j].color_bounds_min, current_clue->sets[j].empty_cache);
			if (clue_options_n == 1) {
				current_clue->last->next = current_clue->next;
				current_clue->next->last = current_clue->last;
				locked_clues[locked_clues_n++] = current_clue;
			}
		}
		if (changes_n && clue_options_min > 0) {
			if (clues_header->next != clues_header) {
				sorted_clues_n = 0;
				for (clue = clues_header->next; clue != clues_header; clue = clue->next) {
					if (clue->priority) {
						sorted_clues[sorted_clues_n++] = clue;
					}
				}
				qsort(sorted_clues, (size_t)sorted_clues_n, sizeof(clue_t *), compare_priorities);
			}
			changes_sum += changes_n;
		}
	}
	while (changes_n && clue_options_min > 0 && clues_header->next != clues_header);
	if (clue_options_min > 0) {
		if (evaluated) {
			clues_header->next != clues_header ? init_option(evaluated, 0, changes_sum):init_option(evaluated, 1, changes_sum);
		}
		else {
			if (verbose || clues_header->next == clues_header) {
				printf("\nTime %us\nAll nodes %u\nRun nodes %u\nFailures %u\nDepth %d Completed %d\nCells %d Locked %u\n", (unsigned)time(NULL)-time_zero, all_nodes_n, run_nodes_n, failures_n, depth, completed_depth, grid_size, locked_cells_n);
				fflush(stdout);
			}
			if (clues_header->next != clues_header) {
				int sorted_sets_n = 0;
				unsigned locked_sets_n_bak = locked_sets_n;
				set_t *set_min;
				for (clue = clues_header->next; clue != clues_header; clue = clue->next) {
					set_t *set;
					for (set = clue->sets_header->next; set != clue->sets_header; set = set->next) {
						if (set->color_bounds_min[DEPTH_CUR] == set->color_bounds_max[DEPTH_CUR]) {
							set->last->next = set->next;
							set->next->last = set->last;
							locked_sets[locked_sets_n++] = set;
						}
					}
					for (set = clue->sets_header->next; set != clue->sets_header; set = set->next) {
						int i;
						set->options_n = 0;
						set->solutions_n = 0;
						set->changes_sum = 0;
						set->skipped = 0;
						for (i = set->color_bounds_min[DEPTH_CUR]; i <= set->color_bounds_max[DEPTH_CUR]; ++i) {
							if (set->color_cache[i-set->color_bounds_min[DEPTH_BCK]] == CACHE_UNKNOWN) {
								set->options[set->options_n++].pos = i;
							}
						}
						set->others_n = set->options_n;
						sorted_sets[sorted_sets_n++] = set;
					}
				}
				do {
					qsort(sorted_sets, (size_t)sorted_sets_n, sizeof(set_t *), compare_sets);
					changes_n = evaluate_set(sorted_sets[0]);
					set_min = init_set_min(sorted_sets[0]);
					if (set_min->others_n > 1) {
						int i;
						for (i = 1; i < sorted_sets_n; ++i) {
							changes_n += evaluate_set(sorted_sets[i]);
							if (compare_evaluations(sorted_sets[i], set_min) < 0) {
								set_min = init_set_min(sorted_sets[i]);
								if (set_min->others_n < 2) {
									break;
								}
							}
						}
					}
					if (verbose) {
						printf("Removed options %d\n", changes_n);
						fflush(stdout);
					}
				}
				while (changes_n && set_min->others_n > 1);
				if (set_min->options_n) {
					int i;
					qsort(set_min->options, (size_t)set_min->options_n, sizeof(option_t), compare_options);
					for (i = set_min->options_n; i--; ) {
						set_min->color_cache[set_min->options[i].pos-set_min->color_bounds_min[DEPTH_BCK]] = negative_cache;
					}
					for (i = set_min->options_n; i--; ) {
						if (depth == completed_depth && i == set_min->options_n-1) {
							completed_depth = depth+1;
						}
						set_min->color_cache[set_min->options[i].pos-set_min->color_bounds_min[DEPTH_BCK]] = CACHE_UNKNOWN;
						++depth;
						--negative_cache;
						nonogram(set_min->clue, NULL);
						++negative_cache;
						--depth;
						set_min->color_cache[set_min->options[i].pos-set_min->color_bounds_min[DEPTH_BCK]] = negative_cache;
					}
					for (i = set_min->options_n; i--; ) {
						set_min->color_cache[set_min->options[i].pos-set_min->color_bounds_min[DEPTH_BCK]] = CACHE_UNKNOWN;
					}
				}
				else {
					failures_n = inc_with_limit(failures_n);
				}
				while (locked_sets_n > locked_sets_n_bak) {
					uncover_set(locked_sets[--locked_sets_n]);
				}
			}
			else {
				int i;
				++solutions_n;
				for (i = 0; i < grid_size; i += width) {
					int j;
					for (j = 0; j < width; ++j) {
						putchar(colors[cells[i+j].color_pos]);
					}
					puts("");
				}
				fflush(stdout);
			}
		}
	}
	else {
		if (evaluated) {
			init_option(evaluated, -1, 0);
		}
	}
	while (locked_clues_n > locked_clues_n_bak) {
		uncover_clue(locked_clues[--locked_clues_n]);
	}
	while (locked_cells_n > locked_cells_n_bak) {
		uncover_cell(locked_cells[--locked_cells_n]);
	}
	for (clue = clues_header->next; clue->pos < width; clue = clue->next) {
		cell_t *cell;
		for (cell = clue->cells_header->column_next; cell != clue->cells_header; cell = cell->column_next) {
			int i;
			for (i = COLOR_POS_EMPTY; i < colors_n; ++i) {
				if (cell->color_cache[i-COLOR_POS_EMPTY] == negative_cache) {
					cell->color_cache[i-COLOR_POS_EMPTY] = CACHE_UNKNOWN;
				}
			}
		}
	}
	for (clue = clues_header->next; clue != clues_header; clue = clue->next) {
		int i;
		for (i = 0; i < clue->sets_n; ++i) {
			clear_set_negative_cache(clue->sets[i].color_bounds_min, clue->sets[i].color_bounds_max, clue->sets[i].color_cache);
			clear_set_negative_cache(clue->sets[i].empty_bounds_min, clue->sets[i].color_bounds_max, clue->sets[i].empty_cache);
			init_color_bounds(clue->sets+i, DEPTH_CUR, clue->sets[i].color_bounds_min[depth+DEPTH_BCK], clue->sets[i].color_bounds_max[depth+DEPTH_BCK]);
			init_empty_bounds(clue->sets+i, DEPTH_CUR, clue->sets[i].empty_bounds_min[depth+DEPTH_BCK]);
		}
		clear_set_negative_cache(clue->sets[i].empty_bounds_min, clue->sets[i].color_bounds_min, clue->sets[i].empty_cache);
		init_empty_bounds(clue->sets+i, DEPTH_CUR, clue->sets[i].empty_bounds_min[depth+DEPTH_BCK]);
	}
}

static void init_option(option_t *option, int r, int changes_sum) {
	option->r = r;
	option->changes_sum = changes_sum;
}

static int reallocate_bounds(set_t *set, int depths_size) {
	int *empty_bounds_min = realloc_mem("set->empty_bounds_min", set->empty_bounds_min, sizeof(int), depths_size), *color_bounds_min, *color_bounds_max;
	if (!empty_bounds_min) {
		return 0;
	}
	set->empty_bounds_min = empty_bounds_min;
	color_bounds_min = realloc_mem("set->color_bounds_min", set->color_bounds_min, sizeof(int), depths_size);
	if (!color_bounds_min) {
		return 0;
	}
	set->color_bounds_min = color_bounds_min;
	color_bounds_max = realloc_mem("set->color_bounds_max", set->color_bounds_max, sizeof(int), depths_size);
	if (!color_bounds_max) {
		return 0;
	}
	set->color_bounds_max = color_bounds_max;
	return 1;
}

static int compare_priorities(const void *a, const void *b) {
	clue_t *clue_a = *(clue_t * const *)a, *clue_b = *(clue_t * const *)b;
	if (clue_a->priority != clue_b->priority) {
		return clue_b->priority-clue_a->priority;
	}
	return clue_a->pos-clue_b->pos;
}

static int sweep_clue(set_t *set, int pos, cell_t *start_cell) {
	int i;
	cell_t *cell;
	for (i = pos, cell = start_cell; i < set->color_bounds_min[DEPTH_CUR] && cell->color_cache[0] >= CACHE_UNKNOWN; ++i, cell += offset);
	if (i >= set->color_bounds_min[DEPTH_CUR]) {
		int r_sum = 0;
		cell_t *last_ok_cell = start_cell;
		if (set != current_clue->sets_header) {
			int j, len = 0, last_ok = i, sum;
			cell_t *colored_cell;
			for (j = i; j <= set->color_bounds_max[DEPTH_CUR]; ++j, cell += offset) {
				if (set->color_cache[j-set->color_bounds_min[DEPTH_BCK]] != CACHE_UNKNOWN) {
					len = 0;
					if (set->color_cache[j-set->color_bounds_min[DEPTH_BCK]] > CACHE_UNKNOWN) {
						r_sum = sum_with_limit(r_sum, set->color_cache[j-set->color_bounds_min[DEPTH_BCK]]);
						last_ok_cell = cell;
						last_ok = j;
					}
					if (cell->color_cache[0] < CACHE_UNKNOWN) {
						++j;
						break;
					}
				}
				else {
					int r;
					if (set->empty_before) {
						if (cell->color_cache[0] >= CACHE_UNKNOWN) {
							r = sweep_set(set, j+1, cell+offset, &colored_cell, &len);
							if (r > 0) {
								cell->color_cache[0] = CACHE_CONFIRMED;
							}
						}
						else {
							len = 0;
							r = negative_cache;
						}
					}
					else {
						r = sweep_set(set, j, cell, &colored_cell, &len);
					}
					if (r > 0) {
						set->color_cache[j-set->color_bounds_min[DEPTH_BCK]] = r;
						r_sum = sum_with_limit(r_sum, r);
						last_ok_cell = cell;
						last_ok = j;
					}
					else {
						set->color_cache[j-set->color_bounds_min[DEPTH_BCK]] = negative_cache;
						if (len > 0) {
							int k;
							if (j+len < set->color_bounds_max[DEPTH_CUR]) {
								k = j+len;
							}
							else {
								if (cell->color_cache[0] < CACHE_UNKNOWN || !r) {
									for (k = set->color_bounds_max[DEPTH_CUR]; k > j; --k) {
										if (set->empty_cache[k-set->empty_bounds_min[DEPTH_BCK]] == CACHE_UNKNOWN) {
											set->empty_cache[k-set->empty_bounds_min[DEPTH_BCK]] = negative_cache;
										}
									}
								}
								k = set->color_bounds_max[DEPTH_CUR];
							}
							for (; k > j; --k) {
								if (set->color_cache[k-set->color_bounds_min[DEPTH_BCK]] == CACHE_UNKNOWN) {
									set->color_cache[k-set->color_bounds_min[DEPTH_BCK]] = negative_cache;
								}
							}
						}
					}
					if (cell->color_cache[0] < CACHE_UNKNOWN || !r) {
						++j;
						break;
					}
					if (len > 0) {
						if (j+len < set->color_bounds_max[DEPTH_CUR]) {
							j += len;
							cell += len*offset;
						}
						else {
							j = set->color_bounds_max[DEPTH_CUR];
						}
					}
				}
			}
			for (--j; j > last_ok; --j) {
				if (set->empty_cache[j-set->empty_bounds_min[DEPTH_BCK]] == CACHE_UNKNOWN) {
					set->empty_cache[j-set->empty_bounds_min[DEPTH_BCK]] = negative_cache;
				}
			}
			sum = 0;
			for (; j > i; --j) {
				if (set->color_cache[j-set->color_bounds_min[DEPTH_BCK]] > CACHE_UNKNOWN) {
					sum = sum_with_limit(sum, set->color_cache[j-set->color_bounds_min[DEPTH_BCK]]);
				}
				set->empty_cache[j-set->empty_bounds_min[DEPTH_BCK]] = sum;
			}
		}
		else {
			++r_sum;
			last_ok_cell = cell;
		}
		for (cell = last_ok_cell-offset; cell >= start_cell; cell -= offset) {
			cell->color_cache[0] = CACHE_CONFIRMED;
		}
		if (r_sum) {
			for (; i >= pos; --i) {
				set->empty_cache[i-set->empty_bounds_min[DEPTH_BCK]] = r_sum;
			}
			return r_sum;
		}
	}
	for (; i >= pos; --i) {
		set->empty_cache[i-set->empty_bounds_min[DEPTH_BCK]] = negative_cache;
	}
	return negative_cache;
}

static int sweep_set(set_t *set, int pos, cell_t *start_cell, cell_t **colored_cell, int *len) {
	int next_pos = pos+set->len, i;
	cell_t *cell;
	if (*len == -1) {
		i = next_pos-1;
		start_cell += (set->len-1)*offset;
	}
	else {
		*colored_cell = NULL;
		i = pos;
	}
	for (cell = start_cell; i < next_pos; ++i, cell += offset) {
		if (cell->color_cache[0] < CACHE_UNKNOWN && !*colored_cell) {
			*colored_cell = cell;
		}
		if (cell->color_cache[set->color_pos-COLOR_POS_EMPTY] < CACHE_UNKNOWN) {
			break;
		}
	}
	if (i == next_pos) {
		int r;
		set_t *next_set = set+1;
		r = next_set->empty_cache[next_pos-next_set->empty_bounds_min[DEPTH_BCK]] != CACHE_UNKNOWN ? next_set->empty_cache[next_pos-next_set->empty_bounds_min[DEPTH_BCK]]:sweep_clue(next_set, next_pos, cell);
		if (r > 0) {
			for (cell -= offset; cell >= start_cell; cell -= offset) {
				cell->color_cache[set->color_pos-COLOR_POS_EMPTY] = CACHE_CONFIRMED;
			}
			*len = -1;
		}
		else {
			*len = 0;
		}
		return r;
	}
	*len = i-pos;
	if (*colored_cell) {
		return 0;
	}
	return negative_cache;
}

static int check_cell_colors(cell_t *cell) {
	int changes_n = 0, color_pos = COLOR_POS_UNKNOWN, i;
	for (i = COLOR_POS_EMPTY; i < colors_n; ++i) {
		if (cell->color_cache[i-COLOR_POS_EMPTY] == CACHE_UNKNOWN) {
			cell->color_cache[i-COLOR_POS_EMPTY] = negative_cache;
			++cell->column_clue->priority;
			++cell->row_clue->priority;
			++changes_n;
		}
		else if (cell->color_cache[i-COLOR_POS_EMPTY] == CACHE_CONFIRMED) {
			cell->color_cache[i-COLOR_POS_EMPTY] = CACHE_UNKNOWN;
			if (color_pos == COLOR_POS_UNKNOWN) {
				color_pos = i;
			}
			else if (color_pos != COLOR_POS_SEVERAL) {
				color_pos = COLOR_POS_SEVERAL;
			}
		}
	}
	if (color_pos > COLOR_POS_SEVERAL) {
		cell->color_pos = color_pos;
		cell->color_cache[color_pos-COLOR_POS_EMPTY] = CACHE_CONFIRMED;
		cell->column_last->column_next = cell->column_next;
		cell->column_next->column_last = cell->column_last;
		cell->row_last->row_next = cell->row_next;
		cell->row_next->row_last = cell->row_last;
		locked_cells[locked_cells_n++] = cell;
	}
	return changes_n;
}

static void update_empty_bound_and_cache(int *bounds_min, const int *bounds_max, int *cache) {
	for (; bounds_min[DEPTH_CUR] <= bounds_max[DEPTH_CUR] && cache[bounds_min[DEPTH_CUR]-bounds_min[DEPTH_BCK]] <= CACHE_UNKNOWN; ++bounds_min[DEPTH_CUR]);
	clear_set_positive_cache(bounds_min, bounds_max, cache);
}

static void update_color_bounds_and_cache(int *bounds_min, int *bounds_max, int *cache) {
	for (; bounds_min[DEPTH_CUR] <= bounds_max[DEPTH_CUR] && cache[bounds_min[DEPTH_CUR]-bounds_min[DEPTH_BCK]] <= CACHE_UNKNOWN; ++bounds_min[DEPTH_CUR]);
	for (; bounds_max[DEPTH_CUR] >= bounds_min[DEPTH_CUR] && cache[bounds_max[DEPTH_CUR]-bounds_min[DEPTH_BCK]] <= CACHE_UNKNOWN; --bounds_max[DEPTH_CUR]);
	clear_set_positive_cache(bounds_min, bounds_max, cache);
}

static void clear_set_positive_cache(const int *bounds_min, const int *bounds_max, int *cache) {
	int i;
	for (i = bounds_min[DEPTH_CUR]; i <= bounds_max[DEPTH_CUR]; ++i) {
		if (cache[i-bounds_min[DEPTH_BCK]] > CACHE_UNKNOWN) {
			cache[i-bounds_min[DEPTH_BCK]] = CACHE_UNKNOWN;
		}
	}
}

static int compare_sets(const void *a, const void *b) {
	int r;
	set_t *set_a = *(set_t * const *)a, *set_b = *(set_t * const *)b;
	r = compare_evaluations(set_a, set_b);
	if (r) {
		return r;
	}
	if (set_a < set_b) {
		return -1;
	}
	return 1;
}

static int evaluate_set(set_t *set) {
	int options_n = set->options_n, i;
	for (i = options_n; i--; ) {
		set->color_cache[set->options[i].pos-set->color_bounds_min[DEPTH_BCK]] = negative_cache;
	}
	set->options_n = 0;
	set->solutions_n = 0;
	set->changes_sum = 0;
	set->skipped = 0;
	i = 0;
	do {
		int j, lo, hi;
		for (j = i+1; j < options_n; ++j) {
			if (set->options[j-1].pos+1 < set->options[j].pos) {
				break;
			}
		}
		for (lo = i; lo < j; ++lo) {
			evaluate_option(set, set->options+lo);
			if (set->options[lo].r >= 0) {
				break;
			}
		}
		for (hi = j-1; hi > lo; --hi) {
			evaluate_option(set, set->options+hi);
			if (set->options[hi].r >= 0) {
				break;
			}
		}
		for (--hi; hi > lo; --hi) {
			init_option(set->options+hi, 0, 0);
			++set->skipped;
		}
		i = j;
	}
	while (i < options_n);
	for (i = 0; i < options_n; ++i) {
		if (set->options[i].r >= 0) {
			set->color_cache[set->options[i].pos-set->color_bounds_min[DEPTH_BCK]] = CACHE_UNKNOWN;
			set->options[set->options_n++] = set->options[i];
		}
	}
	set->others_n = set->options_n-set->solutions_n;
	return options_n-set->options_n;
}

static void evaluate_option(set_t *set, option_t *option) {
	set->color_cache[option->pos-set->color_bounds_min[DEPTH_BCK]] = CACHE_UNKNOWN;
	++depth;
	--negative_cache;
	nonogram(set->clue, option);
	++negative_cache;
	--depth;
	set->color_cache[option->pos-set->color_bounds_min[DEPTH_BCK]] = negative_cache;
	if (option->r >= 0) {
		set->solutions_n += option->r;
		set->changes_sum += option->changes_sum;
	}
}

static set_t *init_set_min(set_t *set) {
	if (verbose) {
		printf("set_min %d %d %d %d\n", set->others_n, set->skipped, set->solutions_n, set->changes_sum);
		fflush(stdout);
	}
	return set;
}

static int compare_evaluations(const set_t *set_a, const set_t *set_b) {
	if (set_a->others_n != set_b->others_n) {
		return set_a->others_n-set_b->others_n;
	}
	if (set_a->skipped != set_b->skipped) {
		return set_a->skipped-set_b->skipped;
	}
	if (set_a->solutions_n != set_b->solutions_n) {
		return set_b->solutions_n-set_a->solutions_n;
	}
	return set_b->changes_sum-set_a->changes_sum;
}

static int compare_options(const void *a, const void *b) {
	const option_t *option_a = (const option_t *)a, *option_b = (const option_t *)b;
	if (option_a->r != option_b->r) {
		return option_a->r-option_b->r;
	}
	if (option_a->changes_sum != option_b->changes_sum) {
		return option_a->changes_sum-option_b->changes_sum;
	}
	return option_b->pos-option_a->pos;
}

static void uncover_set(set_t *set) {
	set->next->last = set;
	set->last->next = set;
}

static void uncover_clue(clue_t *clue) {
	clue->next->last = clue;
	clue->last->next = clue;
}

static void uncover_cell(cell_t *cell) {
	cell->row_next->row_last = cell;
	cell->row_last->row_next = cell;
	cell->column_next->column_last = cell;
	cell->column_last->column_next = cell;
	cell->color_cache[cell->color_pos-COLOR_POS_EMPTY] = CACHE_UNKNOWN;
	cell->color_pos = COLOR_POS_UNKNOWN;
}

static void clear_set_negative_cache(const int *bounds_min, const int *bounds_max, int *cache) {
	int i;
	for (i = bounds_min[depth+DEPTH_BCK]; i <= bounds_max[depth+DEPTH_BCK]; ++i) {
		if (cache[i-bounds_min[DEPTH_BCK]] == negative_cache) {
			cache[i-bounds_min[DEPTH_BCK]] = CACHE_UNKNOWN;
		}
	}
}

static void init_empty_bounds(set_t *set, int pos, int bound_min) {
	set->empty_bounds_min[pos] = bound_min;
}

static void init_color_bounds(set_t *set, int pos, int bound_min, int bound_max) {
	set->color_bounds_min[pos] = bound_min;
	set->color_bounds_max[pos] = bound_max;
}

static unsigned inc_with_limit(unsigned a) {
	if (a < UINT_MAX) {
		return a+1U;
	}
	return UINT_MAX;
}

static int sum_with_limit(int a, int b) {
	if (a <= INT_MAX-b) {
		return a+b;
	}
	return INT_MAX;
}

static void *alloc_mem(const char *name, size_t item_size, int items_n) {
	void *mem = malloc(item_size*(size_t)items_n);
	if (!mem) {
		fprintf(stderr, "Could not allocate memory for %s\n", name);
		fflush(stderr);
	}
	return mem;
}

static void *realloc_mem(const char *name, void *mem, size_t item_size, int items_n) {
	void *reem = realloc(mem, item_size*(size_t)items_n);
	if (!reem) {
		fprintf(stderr, "Could not reallocate memory for %s\n", name);
		fflush(stderr);
	}
	return reem;
}

static void free_data(int cells_max, int clues_max) {
	if (locked_cells) {
		free(locked_cells);
	}
	if (locked_clues) {
		free(locked_clues);
	}
	if (sorted_clues) {
		free(sorted_clues);
	}
	if (locked_sets) {
		free(locked_sets);
	}
	if (sorted_sets) {
		free(sorted_sets);
	}
	if (cells) {
		int i;
		for (i = cells_max; i--; ) {
			free_cell(cells+i);
		}
		free(cells);
	}
	if (clues) {
		int i;
		for (i = clues_max; i--; ) {
			free_clue(clues+i, clues[i].sets_n+1);
		}
		free(clues);
	}
	free(colors);
}

static void free_cell(cell_t *cell) {
	free(cell->color_cache);
}

static void free_clue(clue_t *clue, int sets_max) {
	int i;
	for (i = sets_max; i--; ) {
		free_set(clue->sets+i);
	}
	free(clue->sets);
}

static void free_set(set_t *set) {
	if (set->options) {
		free(set->options);
		set->options = NULL;
	}
	free_ints(&set->color_cache);
	free_ints(&set->empty_cache);
	free_ints(&set->color_bounds_max);
	free_ints(&set->color_bounds_min);
	free_ints(&set->empty_bounds_min);
}

static void free_ints(int **ints) {
	if (*ints) {
		free(*ints);
		*ints = NULL;
	}
}
