#include <stdio.h>
#include <time.h>
#include <limits.h>
#include "quicksort.h"
#include "libcoro.h"

/**
 * You can compile and run this code using the commands:
 *
 * $> gcc solution.c libcoro.c
 * $> ./a.out
 */

struct metaarray {
	int size;
	int* number_array;
};

struct metafile {
	int count;
	int current_file;
	char** filenames;
};

int count_elements(FILE* file){
	int numbers_count = 0;

	int tmp;
	while (!feof(file)) {
		fscanf(file, "%d", &tmp);
		++numbers_count;
	}

	fseek(file, 0, SEEK_SET);
	return numbers_count;
}

void fill_number_array(int* number_array, FILE* file){
	int i = 0;
	while (!feof(file)) {
		fscanf(file, "%d", &number_array[i]);
		++i;
	}

	fseek(file, 0, SEEK_SET);
}

struct my_context {
	int i;
	double latency;
	char* name;
	int* common_array_size;
	struct metafile* metafile;
	struct metaarray** metaarrays;
};

static struct my_context *
my_context_new(int i, double latency, char* name, int* common_array_size, struct metafile* metafile, struct metaarray** metaarrays)
{
	struct my_context *ctx = malloc(sizeof(*ctx));
	ctx->i = i;
	ctx->latency = latency;
	ctx->name = strdup(name);
	ctx->metafile = metafile;
	ctx->common_array_size = common_array_size;
	ctx->metaarrays = metaarrays;
	return ctx;
}

static void
my_context_delete(struct my_context *ctx)
{
	free(ctx->name);
	free(ctx);
}

/**
 * Coroutine body. This code is executed by all the coroutines. Here you
 * implement your solution, sort each individual file.
 */
static int
file_quicksort(void *context)
{		
	struct timespec coro_start, after_initialization, presort_start, sort_start, coro_end;
	clock_gettime(CLOCK_MONOTONIC, &coro_start);

	struct coro *this = coro_this();
	struct my_context *ctx = context;
	float total_cumulative_time;
	char* name = ctx->name;
	float latency = ctx->latency;
	struct metafile* metafile = ctx->metafile;
	struct metaarray** metaarrays = ctx->metaarrays;
	
	clock_gettime(CLOCK_MONOTONIC, &after_initialization);
	total_cumulative_time += (float)(((after_initialization.tv_sec - coro_start.tv_sec) * 1000000000 + (after_initialization.tv_nsec - coro_start.tv_nsec)) / 1000) / 1000000;


	printf("Started coroutine '%s'\n", name);

	while (metafile->current_file < metafile->count) {
		clock_gettime(CLOCK_MONOTONIC, &presort_start);

		int file_index = metafile->current_file;
		char* filename = metafile->filenames[file_index];

		printf("%s: soritng file '%s'\n", name, filename);
		FILE* file = fopen(filename, "r");

		metafile->current_file += 1;
		int numbers_count = count_elements(file);
		*ctx->common_array_size += numbers_count;
		metaarrays[file_index] = malloc(sizeof(struct metaarray));
		metaarrays[file_index]->number_array = malloc(numbers_count * sizeof(int));
		metaarrays[file_index]->size = count_elements(file);
		fill_number_array(metaarrays[file_index]->number_array, file);
		fclose(file);

		clock_gettime(CLOCK_MONOTONIC, &sort_start);
		total_cumulative_time += (float)(((sort_start.tv_sec - presort_start.tv_sec) * 1000000000 + (sort_start.tv_nsec - presort_start.tv_nsec)) / 1000) / 1000000;
		
		quicksort(metaarrays[file_index]->number_array, 0, metaarrays[file_index]->size - 1, latency , &sort_start, &total_cumulative_time);
	}

	clock_gettime(CLOCK_MONOTONIC, &coro_end);
	total_cumulative_time += (float)(((coro_end.tv_sec - sort_start.tv_sec) * 1000000000 + (coro_end.tv_nsec - sort_start.tv_nsec)) / 1000) / 1000000;

	printf("%s: ended after %f s\n", name, total_cumulative_time);
	printf("%s: switch count %lld\n", ctx->name,
			coro_switch_count(this));

	my_context_delete(ctx);
	/* This will be returned from coro_status(). */
	return 0;
}


int
main(int argc, char **argv)
{	
	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);

	int coro_count = atoi(argv[1]);
	float T = atof(argv[2]);
	char* output_filename = "out.txt";
	int common_array_size = 0;
	struct metaarray** metaarrays = malloc((argc - 3) * sizeof(struct metaarray*));
	struct metafile metafile = {argc - 3, 0, &argv[3]};

	coro_sched_init();
	
	for (int i = 0; i < coro_count; ++i) {
		char name[16];
		sprintf(name, "coro_%d", i + 1);
		coro_new(
			file_quicksort, 
			my_context_new(i, (T / coro_count) / 1000000, name, &common_array_size, &metafile, metaarrays));
	}

	struct coro* c;
	while ((c = coro_sched_wait()) != NULL) {
		if (c != NULL){
			printf("Finished with status %d\n", coro_status(c));
			coro_delete(c);
		}
	}

	clock_gettime(CLOCK_MONOTONIC, &end);
	float time_res_tmp = (float)(((end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec)) / 1000) / 1000000;
	printf("Pre merge time %f s\n", time_res_tmp);

    FILE *output_file = fopen(output_filename, "w");
    if (output_file == NULL) {
        exit(EXIT_FAILURE);
    }

	int total_capacity = 0;
    int *mins = (int *)malloc((argc - 3) * sizeof(int));
    int *pos = (int *)malloc((argc - 3) * sizeof(int));
	int min = INT_MAX;
    int io = argc;
    int element_count = 0; 

    for (int i = 0; i < argc - 3; i++) {
        pos[i] = 0;
        mins[i] = metaarrays[i]->number_array[0];
        total_capacity += metaarrays[i]->size;
    }

    while (1){
        for (int i = 0; i < argc - 3; i++) {
            if (mins[i] < min){
                min = mins[i];
                io = i;
            }
        }
        fprintf(output_file, "%d ", min);

        min = INT_MAX;
        ++pos[io];
        if (pos[io] == metaarrays[io]->size){
            mins[io] = INT_MAX;
        } else {
            mins[io] = metaarrays[io]->number_array[pos[io]];
        }
        ++element_count;

        if (element_count == total_capacity){
            break;
        }
    }

	fclose(output_file);

	for (int i = 0; i < argc - 3; ++i){
		free(metaarrays[i]->number_array);
		free(metaarrays[i]);
	}

	free(metaarrays);
	free(mins);
	free(pos);

	clock_gettime(CLOCK_MONOTONIC, &end);
	float time_result = (float)(((end.tv_sec - start.tv_sec) * 1000000000 + (end.tv_nsec - start.tv_nsec)) / 1000) / 1000000;
	printf("Total time is %f s\n", time_result);

	return 0;
}
