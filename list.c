#include "list.h"
/*
 * Appends a socket value into the list
 *
 * Returns -1 if the list isn't initialized else returns 0
 */
int list_append(struct list * list, const void * data) {
	//We don't know if the data is malloced or local Assume local
	//We must copy the data to the heap
	size_t size = list->data_size;

	void * copy = malloc(size);
	memcpy(copy, data, size);

	if (size == 0) {
		return -1;
	}//if

	if (list->length > 0) {
		list->data = realloc(list->data, list->length + 1 * size);
	}//if
	list->data[list->length] = copy;
	list->length++;

	return 0;
}//add

/*
 * Pops the last value off the list
 *
 * Returns the socket fd that was popped on success or -1 on error (there is no value to pop)
 * The result in the list is freed, so the caller is responsible for freeing the
 * popped value
 */
void * list_pop(struct list * list) { 
	size_t size = list->data_size;

	if (size == 0) { //List not initialized
		return NULL;
	}

	if (list->length == 0) {
		puts("length == 0");
		return NULL;
	}
	
	void * popped = malloc(size);
	memcpy(popped, list->data[list->length - 1], size);

	//Free pointer that will be popped
	free(list->data[list->length - 1]);

	list->data = realloc(list->data, list->length * size);
	list->length--;


	return popped;
}//pop

void * list_pop_first(struct list * list) { 
	size_t size = list->data_size;

	if (size == 0) { //List not initialized
		return NULL;
	}

	if (list->length == 0) {
		puts("length == 0");
		return NULL;
	}
	
	void * popped = malloc(size);
	memcpy(popped, list->data[0], size);

	//Free pointer that will be popped
	free(list->data[0]);

	list->data = realloc(list->data, list->length * size);
	list->length--;

	return popped;
}//pop

/*
 * Helper function to remove an element from the array by shifting each element 
 * and removing the last one
 */
void shiftLeft(struct list * list, unsigned int index) {
	for (unsigned int i = index; i < list->length - 1; i++) {
		list->data[i] = list->data[i+1];
	}//for
	list->data = realloc(list->data, list->length * list->data_size);
}//shiftLeft

/*
 * Remove the specified element from the list
 *
 * Returns -1 if the value isn't found
 */
int list_remove(struct list * list, const void * data) {
	if (list->data_size == 0) {
		return 1;
	}//if

	for (unsigned int i = 0; i < list->length; i++) {
		//if (memcmp(data, list->data[i], list->data_size) == 0) {
		if (list->cmp(data, list->data[i])) {
			free(list->data[i]);
			shiftLeft(list, i);
			list->length--;
			return 0;
		}//if
	}//for
	return 1;
}//remove

/*
 * Clean up the memory from the list
 */
void list_destroy(struct list * list) {
	if (list->data_size == 0) { //List not initialized
		return;	
	}//if

	for (unsigned int i = 0; i < list->length; i++) {
		free(list->data[i]);
	}//for
	free(list->data);
}//destroy

void list_printInts(const struct list * list) {
	for (unsigned int i = 0; i < list->length; i++) {
		printf("list[%d] = %d\n", i, *((int *)list->data[i]));
	}//for
}//printInts

void * list_get(const struct list * list, unsigned int index) {
	if (index > list->length) {
		fputs("Error, requested index goes beyond list", stderr);
		return NULL;
	}//if

	if (list->data_size == 0) {
		fputs("Error, list not initialized", stderr);
	}//if

	return list->data[index];
}//get

/*
 * Initializes the list
 */
void list_init(struct list * list, size_t size, int (*cmp)(const void *, const void *)) {
	list->data = malloc(sizeof(void*));
	list->data_size = size;
	list->length = 0;
	list->cmp = cmp;
}//init
