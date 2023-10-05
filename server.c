#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "common.h"


char* ipVersion = "";
char* portNumber = "";
char* filePath = NULL;


// Board used as answer key
int revealedGame[4][4];
// Client's updated current game
int currentGame[4][4];


// Reads the revealed game from a file
void readRevealedGame(char* fileName) {
    FILE *file;
    file = fopen(fileName, "r");
    int pos;
    char comma;

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            fscanf (file, "%d%c", &pos, &comma);
            revealedGame[i][j] = pos;
        }
    }
    fclose(file);
}


// Reset game as all hiddden
void resetGame() {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            currentGame[i][j] = -2;
        }
    }
}


// Reveals specified position based on answer key board
void revealPosition(int coordinates[2]) {
    currentGame[coordinates[0]][coordinates[1]] = revealedGame[coordinates[0]][coordinates[1]];
}


// Inserts flag at the specified position
void flagPosition(int coordinates[2]) {
    currentGame[coordinates[0]][coordinates[1]] = HASFLAG;
}


// Removes flag at the specified position
void removeFlag(int coordinates[2]) {
    currentGame[coordinates[0]][coordinates[1]] = ISSECRET;
}


// Takes the command line arguments for ip version and port number
void captureArgs(int argc, char *argv[]) {
    if (argc != 5 || strcmp(argv[3], "-i" )!= 0) {
        printf("Usage: <ipVersion> <portNumber> -i <filePath>\n");
        return;
    } else {
        ipVersion = argv[1];
        portNumber = argv[2];
        filePath = argv[4];
    }
}


// Verifies if the current game is an won situation
bool wonGame(struct action clientAction) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if(!(clientAction.coordinates[0] == i && clientAction.coordinates[1] == j)
               && (currentGame[i][j] != revealedGame[i][j] && revealedGame[i][j] != HASBOMB)){
                return false;
            }
        }
    }
    return true;
}


// Verifies if, with the client's action, the new board status is game over, won or continue the game
int updateStatus(struct action clientAction) {
    int cellStatus = revealedGame[clientAction.coordinates[0]][clientAction.coordinates[1]];
    if (cellStatus == HASBOMB) {
        return GAMEOVER;
    } else if (wonGame(clientAction)) {
        return WIN;
    }
    return STATE;
}



int main(int argc, char *argv[]) {

    captureArgs(argc, argv);     // Initializes ip version and port number
    readRevealedGame(filePath); // Reads answer key board
    printGame(revealedGame);    // Dumps revealed game

    // Stores information about the IP version and port
    struct sockaddr_storage storage;
    if(server_sockaddr_init(ipVersion, portNumber, &storage)){
        logexit("server_sockaddr_init");
    }

    // Creates socket for TCP communication
    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if(s == -1){
        logexit("socket");
    }

    // Enables reuse mode
    int enable = 1;
    if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0){
        logexit("setsockopt");
    }

    // Bind operation (associates an adress with network socket)
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if(bind(s, addr, sizeof(storage)) != 0){
        logexit("bind");
    }

    // Listen operation (allows incoming connections)
    if(listen(s, 10) != 0){
        logexit("listen");
    }


    while(true) {

        // Accepts an incoming client connection
        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *) &cstorage;
        socklen_t caddrlen = sizeof(cstorage);


        int csock = accept(s, caddr, &caddrlen);
        printf("client connected\n");

        if(csock == -1){
            logexit("accept");
        }


        // Computes client's request
        while(true){
            struct action clientAction;
            // Receives client's command
            int count = recv(csock, &clientAction, sizeof(clientAction), 0);

            if(count == 0){
                break;
            } else if(count == -1){
                logexit("recv");
            }

            struct action serverFeedback;                // Feedback structure sent to the client

            // Creates the structure based on client's request
            switch(clientAction.type) {
                case START:
                    resetGame();
                    int coordinates[2] = {0,0};
                    serverFeedback = computeAction(START, coordinates, currentGame);
                    break;

                case REVEAL:
                    revealPosition(clientAction.coordinates);
                    int updatedStatus = updateStatus(clientAction);

                    if(updatedStatus == WIN){
                        serverFeedback = computeAction(WIN, clientAction.coordinates, currentGame);
                        resetGame();
                    }

                    if (updatedStatus == GAMEOVER) {
                        serverFeedback = computeAction(GAMEOVER, clientAction.coordinates, currentGame);
                        resetGame();
                    }

                    if (updatedStatus == STATE) {
                        serverFeedback = computeAction(STATE, clientAction.coordinates, currentGame);
                    }

                    break;

                case FLAG:
                    flagPosition(clientAction.coordinates);
                    serverFeedback = computeAction(FLAG, clientAction.coordinates, currentGame);
                    break;

                case REMOVE_FLAG:
                    removeFlag(clientAction.coordinates);
                    serverFeedback = computeAction(STATE, clientAction.coordinates, currentGame);
                    break;

                case RESET:
                    resetGame();
                    serverFeedback = computeAction(STATE, clientAction.coordinates, currentGame);
                    break;

                case EXIT:
                    resetGame();
                    printf("client disconnected\n");
                    break;

                default:
                    break;

            }

            // Sends the updated structure with the new board to the client
            count = send(csock, &serverFeedback, sizeof(serverFeedback), 0);
                if(count != sizeof(serverFeedback)){
                    logexit("send");
                }
        }
        close(csock);
    }
    return 0;
}