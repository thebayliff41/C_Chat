#include <stdlib.h>
#include <stdio.h>
#include <sys/socket.h> //sockets1
#include <netinet/in.h> //struct sockaddr_in, struct sockaddr
#include <netdb.h> //getaddrinfo
#include <unistd.h> //close
#include <string.h> //memset strlen
#include <arpa/inet.h> //inet_
#include <pthread.h> //multithreading

#include <ncurses.h> //Screen control

#include "client.h"

int sockfd; 
int rows, cols; //rows = y, cols = x
WINDOW * chat_window; //Where the messages are displayed
WINDOW * prompt_window; //Where the prompt is displayed

int main(int argc, char ** argv) {
	if (initscr() == NULL) { //Start ncurses
		errorAndExit("Can't init ncurses");
	}//if 
	if (atexit(killScreen)) {
		errorAndExit("Error registering exit handler");
	}//if
	getmaxyx(stdscr, rows, cols);
	raw(); //Doesn't create signal on control characters (C-Z)
	noecho(); //Don't print what user types
	//Note, now that multiple windows are being used, we need to use the "w"
	//version of functions to specify which window. Calling the normal refresh()
	//will break the screen (because we don't want to update stdscr)
	
	if ((chat_window = newwin(/*rows - 1*/5, cols, 0, 0)) == NULL) {
		errorAndExit("Can't create chat window");	
	}//if
	if ((prompt_window = newwin(1, cols, rows - 1, 0)) == NULL) {
		errorAndExit("Can't create chat window");	
	}//if

	scrollok(chat_window, TRUE); //Allow scrolling on newline

	if (argc != 2) {
		fprintf(stderr, "USAGE: %s [USER_NAME]\n", *argv);
		exit(1);
	}//if 

	//setup name
	char * name = *(argv + 1);
	if (strlen(name) >= NAMESIZE) { //The name can only be NAMESIZE - 1 characters long
		fprintf(stderr, "Expected name of length %d, but got name of length %lu\n", NAMESIZE - 1, strlen(name));
		exit(1);
	}//if

	struct addrinfo hints, *infoptr, *iterator;
	memset(&hints, 0, sizeof(hints)); //must 0 out all data in struct to avoid garbage data

	//Hints will tell getaddrinfo what to look for
	hints.ai_family = AF_INET; //Get IPV4 Addresses
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	//Store address info into infoptr
	int res;
	if ((res = getaddrinfo("localhost", STR(PORT), &hints, &infoptr)) != 0) {
		printf("%s\n", gai_strerror(res));
		errorAndExit("Error getting address info");
	}//if

	//Get IP address
	char host[INET_ADDRSTRLEN];

	//Find way to connect by iterating over all the connection points returned
	//from getaddrinfo
	for (iterator = infoptr; iterator != NULL; iterator = iterator->ai_next) {
		if ((sockfd = socket(iterator->ai_family, iterator->ai_socktype, iterator->ai_protocol)) < 0) {
			continue;

		}//if
		if (connect(sockfd, iterator->ai_addr, (socklen_t) iterator->ai_addrlen) == 0) {
			getnameinfo(iterator->ai_addr, iterator->ai_addrlen, host, sizeof(host), NULL, 0, NI_NUMERICHOST);
			break;
		} //if

		close(sockfd); 
		sockfd = -1;
	}
	
	if (sockfd < 0) { //no valid socket
		perror("Error, socket could not be created");
		exit(1);
	}//if

	//Once the socket is setup, we want to close it when we exit
	atexit(cleanup);

	//Get the IP address of the server
	inet_ntop(AF_INET, &((struct sockaddr_in *)iterator->ai_addr)->sin_addr, host, INET_ADDRSTRLEN);
	wprintw(chat_window, "IP address of server: %s\nSocket number = %d\n", host, sockfd);
	freeaddrinfo(infoptr);

	wrefresh(chat_window); //Show screen from ncurses

	//The first message should be our name
	send(sockfd, name, strlen(name), 0); 

	pthread_t write;
	pthread_t read;
	if (pthread_create(&write, NULL, sendThread, (void *) &read)) {
		fputs("Error: Unable to create thread", stderr);
		exit(1);
	}//if
	if (pthread_create(&read, NULL, receiveThread, (void *) &write)) {
		fputs("Error: Unable to create thread", stderr);
		exit(1);
	}//if
	//Must wait for both threads to finish so we don't preemptively end the
	//program
	pthread_join(write, NULL);
	pthread_join(read, NULL);
}//main

/*
 * This thread is responsible for receiving data from the server and displaying
 * to the user
 */
void * receiveThread(void * vargp) {
	char message[MSGSIZE];
	ssize_t received_size = 0;
	int cur_row = 0;
	int sub_rows, sub_cols;
	getmaxyx(chat_window, sub_rows, sub_cols);
	while ((received_size = recv(sockfd, message, MSGSIZE, 0)) > 0) {
		if (received_size == -1) { //Error
			perror("Error receiving from server");
			break;
		}//if

		message[received_size] = '\0';

		if (cur_row == sub_rows - 1) { //at the bottom row
			mvwprintw(chat_window, cur_row, 0, "%s\n", message);
		} else {
			mvwprintw(chat_window, cur_row++, 0, "%s\n", message);
		}//if-else
		moveToMessage();
		//Can't refresh the whole screen 
		wrefresh(chat_window);
		wrefresh(prompt_window);
	}//while
	pthread_cancel(*((pthread_t *) vargp));
	pthread_exit(NULL);
}//receiveThread

/*
 * This thread is responsbile for getting user input and sending to ther server
 */
void * sendThread(void * vargp) {
	char message[MSGSIZE]; //Message to send
	size_t message_length;
	size_t total; //total to be sent
	ssize_t sent; //What has been sent so far

	attron(A_BOLD);
	mvwprintw(prompt_window, 0, 0, PROMPT);
	attroff(A_BOLD);
	wrefresh(prompt_window);
	echo(); //print out characters user types

	while (1) {
		wgetstr(prompt_window, message);
		moveToMessage();
		clrtoeol();

		message_length = strlen(message);
		message[message_length] = '\0';

		if (strcmp(message, "exit") == 0) break; //exit closes application
		total = 0;
		while (total != message_length) { //keep sending until full message is sent
			if ((sent = send(sockfd, message + total, message_length - total, 0)) == -1) {
				perror("Error sending to server");
				break;
			}//if
			total += sent;
		}//while
	}//while
	pthread_cancel(*((pthread_t *) vargp));
	pthread_exit(NULL);
}//writeThread

/*
 * Cleanup the program, close the socket
 */
void cleanup() {
	if (close(sockfd)) {
		perror("Error closing socket");
	}//if
	delwin(chat_window);
	delwin(prompt_window);
}//cleanup

void killScreen() {
	endwin();
}//killScreen

void moveToMessage() {
	wmove(prompt_window, 0, strlen(PROMPT));
}//moveToMessage

void writePrompt() {
	attron(A_BOLD);
	mvwprintw(prompt_window, 0, 0, PROMPT);
	attroff(A_BOLD);
	wrefresh(prompt_window);
}//writePrompt
