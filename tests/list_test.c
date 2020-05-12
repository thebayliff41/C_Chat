#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "../list.h"

int main() {
	int x = 5;
	int y = 10;
	int nothing = 0;

	int * d = malloc(sizeof(int));
	d[0] = 7;

	float p = 3.14;
	float z = 10.8;

	struct list ints;
	list_init(&ints, sizeof(int));
	list_append(&ints, (void *) d);
	free(d);
	list_append(&ints, (void *) &y);
	list_printInts(&ints);

	printf("length = %d\n", ints.length);
	//printf("Popped element = %d\n", *((int *)list_pop(&ints)));
	puts("Printing again");
	list_printInts(&ints);
	//printf("Popped element = %d\n", *((int *)list_pop(&ints)));
	printf("length = %d\n", ints.length);
	printf("removing %d: %d\n", x, list_remove(&ints, (void *) &x));
	list_printInts(&ints);
	printf("removing %d: %d\n", nothing, list_remove(&ints, (void *) &nothing));
	list_destroy(&ints);

	sleep(100000);

	//struct list floats;
	//list_init(&floats, sizeof(float));
	//list_append(&floats, (void *) &z);
	//list_append(&floats, (void *) &p);
	//printf("Last element = %f\n", *((float *)list_pop(&floats)));
	//list_destroy(&floats);

	return 0;
}
