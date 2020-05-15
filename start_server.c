#include <unistd.h>
#include <stdio.h>

int main() {
	pid_t pid = fork();
	if (pid == -1) { //error
		perror("Error forking");	
	} else if (pid == 0) { //child
		execl("server", NULL);
		perror("Execl");
	}
}//main
