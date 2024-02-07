GCC_FLAGS = -Wextra -Werror -Wall -Wno-gnu-folding-constant -ldl -rdynamic

all: libcoro.c quicksort.c solution.c ../utils/heap_help/heap_help.c
	gcc $(GCC_FLAGS) libcoro.c quicksort.c solution.c ../utils/heap_help/heap_help.c

clean:
	rm a.out
