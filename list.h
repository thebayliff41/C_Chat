#ifndef SOCKET_LIST
#define SOCKET_LIST
#include <stdlib.h>
#include <stdio.h>
struct socket_list {
	int * sockets;
	int length;
};

//Note: All functions take a pointer, because we are passing by reference
void init(struct socket_list *);
void append(struct socket_list *, int);
int pop(struct socket_list *);
#endif
