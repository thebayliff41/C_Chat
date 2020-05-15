#ifndef SHARING
#define SHARING

#define PIDFILE "pid.txt"
#define LOOPBACK "127.0.0.1"
#define PORT 5556
#define MSGSIZE 512
#define NAMESIZE 20
/*
 * Perror the given message and exit the program with an error code of
 * EXIT_FAILURE 
 */
void errorAndExit(char * error) {
	perror(error);
	exit(EXIT_FAILURE);
}//errorAndExit

#endif
