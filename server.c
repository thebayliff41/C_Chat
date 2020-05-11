#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h> //EXIT_FAILURE
#include <string.h> //memset
#include <netinet/in.h> //struct sockaddr_in, struct sockaddr
#include <arpa/inet.h> //inet_aton()
#include <unistd.h> //close()
#include <signal.h> //signal handler
#include <pthread.h> //Multithreading

#include "list.h" //socket_list

#define LOOPBACK 127.0.0.1
#define PORT 5556

struct thread_argument {
	int socket;
	int finished;
	pthread_mutex_t finish_lock;
};

struct custom_thread {
	pthread_t thread;
	struct thread_argument * args;
}; //custom_thread


void errorAndExit(char*);
void cleanup(int); 
void * handleThread(void*);
void * manageThreads(void *);

struct list thread_list; //global list of sockets in use
pthread_mutex_t list_lock;

int server_socket; //The socket that clinets will connect to

int main(int argc, char** argv) {
	int yes = 1; //Pointer used in setsockopt
	struct sockaddr_in host, client; //Internet socket addresses (subset of socket addresses)

	//Create socket utilizing TCP
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		errorAndExit("Creation of socket failed");
	}//if

	//Set socket-level options to allow the reuse of the address and port. This
	//avoids "address already in use" error
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &yes, sizeof(yes)) == 0) {
		errorAndExit("Failed to set socket options");
	}//if

	host.sin_family = AF_INET; //We will communicate with IPV4 Addresses.
	host.sin_port = htons(PORT); //Network byte order port value of what the client will connnect to
	host.sin_addr.s_addr = 0; //Host IP. 0 Will automatically get filled in.
	memset(host.sin_zero, '\0', strlen(host.sin_zero)); 

	//Bind the socket to the port and address given ^
	if (bind(server_socket, (struct sockaddr *)&host, sizeof(host)) == -1) {
		errorAndExit("Failed ot bind to host");
	}//if

	//Listen for attempted connection on the address and the port. Maximum of 5
	//waiting connections at a time. 
	if (listen(server_socket, 5) == -1) {
		errorAndExit("Failed to start listening on host");
	}//if

	//ntohs converts network-readable value to host-readable value
	printf("Sever: Server started on port: %d\n", ntohs(host.sin_port));
	signal(SIGINT, cleanup); //Keyboard interrupt will be handled correctly

	list_init(&thread_list, sizeof(struct custom_thread));

	pthread_mutex_init(&list_lock, NULL);

	pthread_t manager_thread;
	if (pthread_create(&manager_thread, NULL, manageThreads, NULL)) {
		fputs("Error: Unable to create thread", stderr);
	}//if
	
	//Loop over accepting new connections
	while (1) {
		socklen_t sock_size = sizeof(struct sockaddr_in); //Get length of internet socket, needed when accepting new connection
		int client_sock = accept(server_socket, (struct sockaddr *) &client, &sock_size); //Accept new connection and fill in client details
		if (client_sock == -1) {
			perror("Client failed to connect");
			continue;
		}//if

		//Note: client side port numbers aren't important. Typically, the system
		//will assign these randomly
		printf("Server: Received connection from %s on port %d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));

		//list_append(&thread_list, (void *) &client_sock);
	
		//Give each client their own thread
		struct custom_thread * thread = malloc(sizeof(struct custom_thread));
		struct thread_argument * args = malloc(sizeof(struct thread_argument));

		//struct custom_thread thread;
		//struct thread_argument args;
		
		//Init values
		args->socket = client_sock;
		args->finished = 0;
		thread->args = args;
		printf("Thread->args->finished = %d\n", thread->args->finished);
		
		//args.socket = client_sock;
		//args.finished = 0;
		//thread.args = &args;
		//pthread_mutex_init(&(args.finish_lock), NULL);

		pthread_mutex_lock(&list_lock);
		list_append(&thread_list, (void *) thread);
		pthread_mutex_unlock(&list_lock);

		if (pthread_create(&(thread->thread), NULL, handleThread, (void *) args)) {
		//if (pthread_create(&(thread.thread), NULL, handleThread, (void *) args)) {
			fputs("Error: Unable to create thread", stderr);
		}//if

		//close(client_sock);
		//puts("Clinet socket closed");

		//if (pop(&socket_list) == -1) {
		//	fputs("Failed to pop from the socket list.", stderr);
		//}//if
	}//while
	close(server_socket);
	list_destroy(&thread_list);
}//main

/*
 * Thread that manages other running threads. Checks every second if a
 * connection has been interrupted.
 */
void * manageThreads(void * vargp) {
	while(1) {
		pthread_mutex_lock(&list_lock);
		puts("here");
		for (unsigned int i = 0; i < thread_list.length; i++) {
			struct custom_thread * thread = list_get(&thread_list, i);
			pthread_mutex_lock(&(thread->args->finish_lock));
			printf("Thread->args->finished = %d\n", thread->args->finished);
			if (thread->args->finished) { //Free memory once thread is done
				puts("Thread finished");
				close(thread->args->socket);
				list_remove(&thread_list, thread);
				pthread_mutex_unlock(&(thread->args->finish_lock));
				//free(thread->args);
				//free(thread);
			} else {
				pthread_mutex_unlock(&(thread->args->finish_lock));
			}//if
		}//for
		pthread_mutex_unlock(&list_lock);
		sleep(1);
	}//while
}//manageThreads

/*
 * Function to handle the threads created.
 */
void * handleThread(void *vargp) {
	struct thread_argument * args = (struct thread_argument*) vargp;
	printf("I started a thread with a socket number of %d\n", args->socket);

	//Lock finished while writing to it
	pthread_mutex_lock(&(args->finish_lock));
	puts("finishing");
	args->finished = 1;
	sleep(5000);
	puts("slept for 10");
	pthread_mutex_unlock(&(args->finish_lock));

	pthread_exit(NULL);
}//handleThread

/*
 * Perror the given message and exit the program with an error code of
 * EXIT_FAILURE 
 */
void errorAndExit(char * error) {
	perror(error);
	exit(EXIT_FAILURE);
}//errorAndExit

/*
 * If there is a keyboard interrupt, the connection should be closed gracefully
 */
void cleanup(int signal) {
	//If in the middle of a connection, close the connection safely
	//if (thread_list.length > 0)
	//	close(*((int *)list_pop(&thread_list)));
	list_destroy(&thread_list);
	close(server_socket);
	exit(0);
}//cleanupSocket
