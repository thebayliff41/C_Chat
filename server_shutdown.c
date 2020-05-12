#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "sharing.h"

int main() {
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

	kill(pid, SIGUSR1);
}//main
