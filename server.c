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
#include "sharing.h" //PIDFILE

#define LOOPBACK 127.0.0.1
#define PORT 5556
#define MSGSIZE 512

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
void cleanup(); 
void * handleThread(void*);
void * manageThreads(void *);
void interruptHandler(int);

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

	//place pid in file
	FILE * pid_file;
	if ((pid_file = fopen(PIDFILE, "w")) == NULL) {
		errorAndExit("Failed to write to pid file");
	}//if
	fprintf(pid_file, "%d", getpid());
	if (fclose(pid_file)) {
		errorAndExit("Failed to close file");
	}//if

	atexit(cleanup);

	//ntohs converts network-readable value to host-readable value
	printf("Sever: Server started on port: %d\n", ntohs(host.sin_port));
	signal(SIGUSR1, interruptHandler); //USR1 interrupt will be used to init shutdown

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

		//Give each client their own thread
		struct custom_thread * thread = malloc(sizeof(struct custom_thread));
		struct thread_argument * args = malloc(sizeof(struct thread_argument));

		//Init values
		args->socket = client_sock;
		args->finished = 0;
		thread->args = args;
		pthread_mutex_init(&(args->finish_lock), NULL);

		pthread_mutex_lock(&list_lock);
		list_append(&thread_list, (void *) thread);
		pthread_mutex_unlock(&list_lock);

		if (pthread_create(&(thread->thread), NULL, handleThread, (void *) args)) {
			fputs("Error: Unable to create thread", stderr);
		}//if

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
		for (unsigned int i = 0; i < thread_list.length; i++) {
			struct custom_thread * thread = list_get(&thread_list, i);

			//We don't want to block and wait for the thread to finish
			if (pthread_mutex_trylock(&(thread->args->finish_lock))) {
				continue;
			}//if

			int finished = thread->args->finished;
			pthread_mutex_unlock(&(thread->args->finish_lock));
			if (finished) { //Free memory once thread is done
				//Once the thread says it's done, wait for the thread to finish whatever
				//it may be doing
				
				pthread_join(thread->thread, NULL);

				//We need to free the args, but the list will handle freeing the thread
				free(thread->args);
				if(list_remove(&thread_list, thread)) {
					fputs("Failed to remove thread from list", stderr);
				}//if
			}//if
		}//for
		pthread_mutex_unlock(&list_lock);
		sleep(1); //We don't want to poll too often
	}//while
}//manageThreads

/*
 * Function to handle the threads created.
 */
void * handleThread(void *vargp) {
	struct thread_argument * args = (struct thread_argument*) vargp;
	char message[MSGSIZE];
	printf("I started a thread with a socket number of %d\n", args->socket);

	ssize_t received_size = 0;
	while((received_size = recv(args->socket, message, MSGSIZE, 0))) {
		if (received_size == -1) { //Error
			perror("Error receiving from client");
			break;
		}//if
		memset(message + received_size, '\0', MSGSIZE - received_size);
		printf("Message from client: %s", message);
		ssize_t send_size;
		if((send_size = send(args->socket, "I got it!\n", 10, 0)) == -1) {
			perror("Error sending to client");
		}//if
	}//while
	puts("Finished with client");

	//Lock finished while writing to it
	pthread_mutex_lock(&(args->finish_lock));
	args->finished = 1;
	pthread_mutex_unlock(&(args->finish_lock));

	close(args->socket);
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
 */
void cleanup() {
	list_destroy(&thread_list);
	close(server_socket);
	remove(PIDFILE);
}//cleanupSocket

/*
 * If there is a keyboard interrupt, the connection should be closed gracefully
 */
void interruptHandler(int sig) {
	exit(0);
}//interruptHandler
