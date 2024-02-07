#pragma once

#include <stdlib.h>
#include <string.h>
#include <time.h>

void quicksort(int* array, int low, int high, float latency, struct timespec* sort_start, float* yeild_delay_time);
