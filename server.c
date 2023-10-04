#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "common.h"


char* ipVersion = "";
char* portNumber = "";
char* filePath = NULL;


// todas posicoes reveladas
int revealedGame[4][4];
// jogo do cliente em tempo real
int currentGame[4][4];


// Le arquivo do terminal e preenche matriz global com o resultado
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


// reseta o jogo todo pra -
void resetGame() {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            currentGame[i][j] = -2;
        }
    }
}


// Recebe coordenadas e revela no tabuleiro do cliente o certo
void revealPosition(int coordinates[2]) {
    currentGame[coordinates[0]][coordinates[1]] = revealedGame[coordinates[0]][coordinates[1]];
}


// Coloca flag em uma posicao
void flagPosition(int coordinates[2]) {
    currentGame[coordinates[0]][coordinates[1]] = HASFLAG;
}


// Tira flag de uma posicao
void removeFlag(int coordinates[2]) {
    currentGame[coordinates[0]][coordinates[1]] = ISSECRET;
}


// le o terminal
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

    captureArgs(argc, argv);
    readRevealedGame(filePath); // preenche o tabuleiro gabarito
    printGame(revealedGame);    // servidor printa o resultado quando inicia

    struct sockaddr_storage storage;
    if(server_sockaddr_init(ipVersion, portNumber, &storage)){
        logexit("server_sockaddr_init");
    }




    // socketssss ----------------------------------------------------
    // Socket
    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if(s == -1){
        logexit("socket");
    }

    // Reuse
    int enable = 1;
    if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) != 0){
        logexit("setsockopt");
    }

    // Bind
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if(bind(s, addr, sizeof(storage)) != 0){
        logexit("bind");
    }

    // Listen
    if(listen(s, 10) != 0){
        logexit("listen");
    }



    while(true) {

        struct sockaddr_storage cstorage;
        struct sockaddr *caddr = (struct sockaddr *) &cstorage;
        socklen_t caddrlen = sizeof(cstorage);

        int csock = accept(s, caddr, &caddrlen);
        printf("client connected\n");

        if(csock == -1){
            logexit("accept");
        }


        while(true){
            struct action clientAction;
            int count = recv(csock, &clientAction, sizeof(clientAction), 0);
            printf("client typee:  %d\n",clientAction.type);

            if(count == 0){
                break;
            } else if(count == -1){
                logexit("recv");
            }

            struct action serverFeedback;


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

            count = send(csock, &serverFeedback, sizeof(serverFeedback), 0);
                if(count != sizeof(serverFeedback)){
                    logexit("send");
                }
        }
        close(csock);
    }
    return 0;
}
