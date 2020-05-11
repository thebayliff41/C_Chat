#include "list.h"
/*
 * Appends a socket value into the list
 */
void append(struct socket_list * list, int sock) {
	if (list->length == 0) {
		init(list);
	}//if

	list->sockets[list->length] = sock;
	list->sockets = realloc(list->sockets, list->length + 1);
	list->length++;
}//add

/*
 * Pops the last value off the list
 *
 * Returns the socket fd that was popped on success or -1 on error (there is no value to pop)
 */
int pop(struct socket_list * list) {
	if (list->length == 0) {
		return -1;
	}

	int popped = list->sockets[list->length];
	list->sockets = realloc(list->sockets, list->length - 1);
	list->length--;

	return popped;
}//pop

/*
 * Initializes the list
 */
void init(struct socket_list * list) {
	list->sockets = malloc(sizeof(int));
	list->length = 0;
}//init
