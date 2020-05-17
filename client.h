#ifndef CLIENT_H
#define CLIENT_H

#define STR_(x) #x
#define STR(x) STR_(x)
#define PROMPT "Enter Message: "

#include "sharing.h"

void * receiveThread(void *);
void * sendThread(void *);
void cleanup(void);
void killScreen(void);
void moveToMessage(void);
void writePrompt(void);

#endif
