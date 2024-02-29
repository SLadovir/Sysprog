GCC_FLAGS = -Wextra -Werror -Wall -Wno-gnu-folding-constant -ldl -rdynamic

all: solution.c ../utils/heap_help/heap_help.c
	gcc $(GCC_FLAGS) solution.c ../utils/heap_help/heap_help.c

clean:
	rm a.out
