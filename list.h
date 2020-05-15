#ifndef LIST
#define LIST
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
struct list {
	void ** data;
	size_t data_size; //Necessary?
	unsigned int length;
	int initialized;
	int (*cmp) (const void*, const void*);
};

//Note: All functions take a pointer, because we are passing by reference
void list_init(struct list *, size_t, int (*) (const void*, const void*));
int list_append(struct list *, const void *);
void * list_pop(struct list *);
int list_remove(struct list *, const void *);
void list_destroy(struct list *); 
void list_printInts(const struct list *);
void * list_get(const struct list *, unsigned int);
#endif
