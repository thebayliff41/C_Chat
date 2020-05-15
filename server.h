#ifndef SERVER
#define SERVER

#define LOGFILE ".log"
#define TIMESIZE 20

struct thread_argument {
	int socket;
	int finished;
	pthread_mutex_t finish_lock;
	char name[NAMESIZE];
};

struct custom_thread { 
	pthread_t thread;
	struct thread_argument * args;
}; //custom_thread

void cleanup(void); 
void * handleThread(void*);
void * manageThreads(void *);
void interruptHandler(int);
void updateClients(const char*, int, int);
void printPonters(void);
void logAndExit(const char*);
void logm(const char*);
#endif
