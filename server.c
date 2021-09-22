#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h> //EXIT_FAILURE
#include <string.h> //memset
#include <netinet/in.h> //struct sockaddr_in, struct sockaddr
#include <arpa/inet.h> //inet_aton()
#include <unistd.h> //close()
#include <signal.h> //signal handler
#include <pthread.h> //Multithreading
#include <ctype.h> //isgraph()
#include <sys/time.h> //gettimeofday
#include <time.h> //localtime

#include "list.h" //thread_list
#include "sharing.h" //PIDFILE, other defines
#include "server.h"

struct list thread_list;
pthread_mutex_t list_lock;

struct list update_list;
//Different semaphore
pthread_t update;

int server_socket; //The socket that clients will connect to

FILE * log_file; //log file

//How to compare the custom threads. If they share the same socket, then they
//have to be the same.
int compareThreads(const void * p1, const void * p2) {
	const struct custom_thread * t1 = (struct custom_thread *) p1;
	const struct custom_thread * t2 = (struct custom_thread *) p2;
	return t1->args->socket == t2->args->socket;
}//compareThreads

int main(int argc, char** argv) {
	if ((log_file = fopen(LOGFILE, "a")) == NULL) { //Open log file
		fputs("Error opening log file", stderr);
		exit(1);
	}//if

	struct sockaddr_in host, client; //Internet socket addresses (subset of socket addresses)

	//Create socket utilizing TCP
	if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		logAndExit("Creation of socket failed");
	}//if

	//Set socket-level options to allow the reuse of the address and port. This
	//avoids "address already in use" error
	if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &(int){1}, sizeof(int)) < 0) {
		logAndExit("Failed to set socket options");
	}//if

	host.sin_family = AF_INET; //We will communicate with IPV4 Addresses.
	host.sin_port = htons(PORT); //Network byte order port value of what the client will connnect to
	host.sin_addr.s_addr = 0; //Host IP. 0 Will automatically get filled in.
	memset(host.sin_zero, '\0', strlen(host.sin_zero)); 

	//Bind the socket to the port and address given ^
	if (bind(server_socket, (struct sockaddr *)&host, sizeof(host)) == -1) {
		logAndExit("Failed to bind to host");
	}//if

	//Listen for attempted connection on the address and the port. Maximum of 5
	//waiting connections at a time. 
	if (listen(server_socket, 5) == -1) {
		logAndExit("Failed to start listening on host");
	}//if

	//place pid in file
	FILE * pid_file;
	if ((pid_file = fopen(PIDFILE, "w")) == NULL) {
		logAndExit("Failed open pid file");
	}//if

	fprintf(pid_file, "%d", getpid());

	if (fclose(pid_file)) {
		logAndExit("Failed to close file");
	}//if

	atexit(cleanup);

	//ntohs converts network-readable value to host-readable value
	char log_message[100];
	sprintf(log_message, "Sever: Server started on port: %d", ntohs(host.sin_port));
	logm(log_message);
	signal(SIGUSR1, interruptHandler); //USR1 interrupt will be used to init shutdown

	list_init(&thread_list, sizeof(struct custom_thread), compareThreads);
	//init sem here
	pthread_mutex_init(&list_lock, NULL);

	pthread_t manager_thread;
	if (pthread_create(&manager_thread, NULL, manageThreads, NULL)) {
		logAndExit("Error: Unable to create thread");
	}//if
	
	//Loop over accepting new connections
	while (1) {
		socklen_t sock_size = sizeof(struct sockaddr_in); //Get length of internet socket, needed when accepting new connection
		int client_sock = accept(server_socket, (struct sockaddr *) &client, &sock_size); //Accept new connection and fill in client details
		if (client_sock == -1) {
			logm("Client failed to connect");
			continue;
		}//if

		//Note: client side port numbers aren't important. Typically, the system
		//will assign these randomly
		char client_address[INET_ADDRSTRLEN];

		if (inet_ntop(AF_INET, &(client.sin_addr), client_address, INET_ADDRSTRLEN) == NULL) {
			logm("Error getting string address using inet_ntop");
		} else {
			sprintf(log_message, "Server: Received connection from %s on port %d", client_address, ntohs(client.sin_port));
			logm(log_message);
		} //if-else

		//Give each client their own thread
		struct custom_thread * thread = malloc(sizeof(struct custom_thread));
		if (thread == NULL) {
			logAndExit("Error in malloc");	
		}//if
		struct thread_argument * args = malloc(sizeof(struct thread_argument));
		if (args == NULL) {
			free(thread);
			logAndExit("Error in malloc");	
		}//if

		//Init values
		args->socket = client_sock;
		args->finished = 0;
		thread->args = args;
		pthread_mutex_init(&(args->finish_lock), NULL);

		pthread_mutex_lock(&list_lock);
		list_append(&thread_list, (void *) thread);
		pthread_mutex_unlock(&list_lock);

		if (pthread_create(&(thread->thread), NULL, handleThread, (void *) args)) {
			free(thread);
			logAndExit("Error: Unable to create thread");
		}//if

		free(thread); //Because the list re-mallocs the thread

	}//while
}//main

void printPointers() {
	pthread_mutex_lock(&list_lock);
	for (unsigned int i = 0; i < thread_list.length; i++) {
		struct custom_thread * thread = list_get(&thread_list, i);
		printf("thread pointer: %p, args pointer %p\n", (void *)thread, (void *)thread->args);
	}//for
	pthread_mutex_unlock(&list_lock);
}//printPointers

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
				//it may be doing for cleanup
				pthread_join(thread->thread, NULL);

				//We need to free the args, but the list will handle freeing the thread
				free(thread->args);
				if(list_remove(&thread_list, thread)) {
					logm("Failed to remove thread from list");
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
	char broadcast[MSGSIZE + NAMESIZE + 2];
	ssize_t received_size = 0;

	received_size = recv(args->socket, args->name, NAMESIZE - 1, 0);
	args->name[received_size] = '\0';
	int name_length = strlen(args->name);
	//the broadcast always starts with [name]:[space]
	strcpy(broadcast, args->name);
	strcpy(broadcast + name_length, ": ");

	sprintf(message, "I started a thread with a socket number of %d, their name is %s", args->socket, args->name);
	logm(message);

	while((received_size = recv(args->socket, message, MSGSIZE, 0))) {
		if (received_size == -1) { //Error
			logm("Error receiving from client");
			break;
		}//if
		message[received_size] = '\0';

		//Remove special characters from string
		char * c = message;
		for (char * m = message; *m != '\0'; m++) {
			if (isprint(*m)) {
				*c++ = *m;
			}//if
		}//for
		*c = '\0';
		
		//printf("Server: Message from client: %s\n", message);
		strcpy(broadcast + name_length + 2, message);
		//broadcast[name_length + received_size + 2] = '\0'; //strcpy already
		//convers \0
		//Convert to it's own thread (using a semaphore)
		updateClients(broadcast, args->socket, strlen(broadcast));
	}//while

	//Lock finished while writing to it
	pthread_mutex_lock(&(args->finish_lock));
	args->finished = 1;
	pthread_mutex_unlock(&(args->finish_lock));

	close(args->socket);
	pthread_exit(NULL);
}//handleThread


/*
 * Send specified message to all other connected clients.
 */
void updateClients(const char * message, int original_socket, int message_length) {
	pthread_mutex_lock(&list_lock);
	for (unsigned int i = 0; i < thread_list.length; i++) {
		struct custom_thread * thread = list_get(&thread_list, i);
		if (thread->args->socket == original_socket) { //don't send to original socket
			continue;
		}//if

		ssize_t total = 0;
		ssize_t sent;
		while(total != message_length) {
			if ((sent = send(thread->args->socket, message, message_length, 0)) == -1) {
				logm("Error sending to client");
				break;
			}//if
			total += sent;
		}//while
	}//for
	pthread_mutex_unlock(&list_lock);
}//updateClients

/*
 * Cleanup the server -- Free the memory, close the server socket, remove the
 * temporary file
 */
void cleanup() {
	pthread_mutex_lock(&list_lock);
	for (unsigned int i = 0; i < thread_list.length; i++) {
		struct custom_thread * thread = list_get(&thread_list, i);

		pthread_cancel(thread->thread);
		pthread_join(thread->thread, NULL);

		//We need to free the args, but the list will handle freeing the thread
		free(thread->args);
		if(list_remove(&thread_list, thread)) {
			logm("Failed to remove thread from list");
		}//if
	}//for
	pthread_mutex_unlock(&list_lock);
	if (close(server_socket)) {
		logm("Error closing server_socket");
	}//if

	if (remove(PIDFILE)) {
		logm("Unable to remove PIDFILE");
	}//if

	logm("Shutting down server");

	if (fclose(log_file)) {
		perror("Error closing log file");
	}//if
}//cleanupSocket

/*
 * If there is a keyboard interrupt, the connection should be closed gracefully
 */
void interruptHandler(int sig) {
	exit(0);
}//interruptHandler

void logAndExit(const char * log_message) {
	logm(log_message);
	exit(1);
}//logAndExit

void logm(const char * log_message) {
	struct timeval time;
	char time_string[TIMESIZE] = "Unknown Time: "; //mm/dd/yy_hh:mm:ss:_\0
	if (gettimeofday(&time, NULL)) { //Error
		fputs("Error getting time of day", log_file);
	} else { //Success
		struct tm * now = localtime(&(time.tv_sec));
		strftime(time_string, TIMESIZE, "%m/%d/%y %T: ", now); //Convert time to string and store in time_string
	}//if-else

	fprintf(log_file, "%s%s\n", time_string, log_message);
	//fprintf(log_file, "%s\n", log_message);
}//log
