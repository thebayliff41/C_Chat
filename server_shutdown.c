#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "sharing.h"

int main() {
	//get PIO from file
	FILE * pid_file;
	if ((pid_file = fopen(PIDFILE, "r")) == NULL) {
		perror("Can't open file");
		exit(1);
	}//if
	int pid = -1;
	fscanf(pid_file, "%d", &pid);

	if (pid == -1) {
		fputs("Error reading file", stderr);
		exit(1);
	}//if

	//Send shutdown signal to server
	kill(pid, SIGUSR1);
}//main
