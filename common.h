struct action {
    int type;
    int coordinates[2];
    int board[4][4];
};

#pragma once

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
// socket libraries
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>

// Game status
#define ONGOING 1
#define ENDGAME 0

// Cell options
#define ISCLEAN 0
#define HASBOMB -1
#define ISSECRET -2
#define HASFLAG -3

// Action types
#define ERROR -5
#define UNKNOWN -1
#define START 0
#define REVEAL 1
#define FLAG 2
#define STATE 3
#define REMOVE_FLAG 4
#define RESET 5
#define WIN 6
#define EXIT 7
#define GAMEOVER 8

// Error types
#define COMMANDNOTFOUND "error: command not found"
#define INVALIDCELL "error: invalid cell"
#define FLAGGEDCELL "error: cell already has a flag"
#define FLAGREVEALEDCELL "error: cannot insert flag in revealed cell"
#define ALREADYREVEALED "error: cell already revealed"


struct action computeAction(int type, int coordinates[2], int board[4][4]);
void addrtostr(const struct sockaddr *addr, char *str, size_t strsize);
int addrparse(const char *addrstr, const char *portstr, struct sockaddr_storage *storage);
int server_sockaddr_init(const char *protocol_version, const char *portstr, struct sockaddr_storage *storage);
void logexit(const char *msg);
void printGame(int matrix[4][4]);
void errorMessage(char* error);