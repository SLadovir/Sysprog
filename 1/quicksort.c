#include <stdio.h>
#include "libcoro.h"
#include "quicksort.h"


void quicksort(int* array, int low, int high, float latency, struct timespec* sort_start, float* total_cumulative_time) {
    if (low < high) {
        int pivot = array[high];
        int i = low - 1;

        for (int j = low; j <= high - 1; ++j) {
            if (array[j] <= pivot) {
                ++i;
                // swap array[i] and array[j]
                int temp = array[i];
                array[i] = array[j];
                array[j] = temp;
            }
        }

        // swap array[i+1] and array[high] (pivot)
        int temp = array[i + 1];
        array[i + 1] = array[high];
        array[high] = temp;

        int partition_index = i + 1;

        // Рекурсивная сортировка элементов до и после опорного элемента
        quicksort(array, low, partition_index - 1, latency, sort_start, total_cumulative_time);
        quicksort(array, partition_index + 1, high, latency, sort_start, total_cumulative_time);

		struct timespec current_time;
		clock_gettime(CLOCK_MONOTONIC, &current_time);

		float duration = (float)(((current_time.tv_sec - sort_start->tv_sec) * 1000000000 + (current_time.tv_nsec - sort_start->tv_nsec)) / 1000) / 1000000;

		if (duration >= latency){
			*total_cumulative_time += duration;
			coro_yield();
			clock_gettime(CLOCK_MONOTONIC, sort_start);
		}
    }
}