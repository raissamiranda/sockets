#include "common.h"



int currentGame[4][4];



char *ipVersion = "";
char *port = "";
int gameStatus;
int isAvailable = 1;



// Le os argumentos passados pelo cliente (./bla ip port)
void computeArgs(int argc, char *argv[]) {
    if (argc != 3) {
        return 1;
    } else {
        ipVersion = argv[1];
        port = argv[2];
    }
}


// inicializa o currentGame antes de qualquer comando ser enviado como tudo -1
void initCurrentGame() {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            currentGame[i][j] =  DEFAULT;
        }
    }
}




// Retorna o tipo do comando colocado pelo cliente
int computeCmdType(char *cmd) {
    char *command = strtok(cmd, " ");
    int cmdType;

    if (command == NULL) {
        return UNKNOWN;
    }

    switch(command[0]) {
        case 's':
            if(strcmp(command, "start") == 0) {
                cmdType = START;
            } else {
                cmdType = UNKNOWN;
            }
            break;

        case 'r':
            if (strcmp(command, "reveal") == 0) {
                cmdType = REVEAL;
            } else if (strcmp(command, "remove_flag") == 0) {
                cmdType = REMOVEFLAG;
            } else if (strcmp(command, "reset") == 0) {
                cmdType = RESET;
            } else {
                cmdType = UNKNOWN;
            }
            break;

        case 'f':
            if (strcmp(command, "flag") == 0) {
                cmdType = FLAG;
            } else {
                cmdType = UNKNOWN;
            }
            break;

        case 'e':
            if (strcmp(command, "exit") == 0) {
                cmdType = EXIT;
            } else {
                cmdType = UNKNOWN;
            }
            break;

        default:
            cmdType = UNKNOWN;
            break;
    }

    return cmdType;
}




bool accepted(int coordinates[2]){
    return coordinates[0] < 4 && coordinates[0] >= 0 &&
           coordinates[1] < 4 && coordinates[1] >= 0;
}



bool isFlagged(int coordinates[2]){
    return currentGame[coordinates[0]][coordinates[1]] == FLAG;
}



bool isRevealed(int coordinates[2]){
    return currentGame[coordinates[0]][coordinates[1]] >= 0;
}



// chama fluxo especifico dependendo do comando do cliente
struct action computeClientAction(int cmdType, int coordinates[2]) {
    char comma;

    switch(cmdType){

        case START:
            gameStatus = ONGOING;
            return computeAction(START, coordinates, currentGame);
            printf("game started");

        case REVEAL:
            scanf("%d%c%d", &coordinates[0], &comma, &coordinates[1]);
            if (!accepted(coordinates)) {
                errorMessage(INVALIDCELL);
                isAvailable = 0;
                break;
            } else if (isRevealed) {
                errorMessage(ALREADYREVEALED);
                isAvailable = 0;
                break;
            }
            return computeAction(REVEAL, coordinates, currentGame);

        case FLAG:
            scanf("%d%c%d", &coordinates[0], &comma, &coordinates[1]);
            if (isFlagged(coordinates)) {
                errorMessage(FLAGGEDCELL);
                isAvailable = 0;
                break;
            } else if (isRevealed(coordinates)){
                errorMessage(FLAGREVEALEDCELL);
                isAvailable = 0;
                break;
            }
            return computeAction(FLAG, coordinates, currentGame);

        case REMOVEFLAG:
            scanf("%d%c%d", &coordinates[0], &comma, &coordinates[1]);
            if (!accepted(coordinates)) {
                isAvailable = 0;
                break;
            }
            return computeAction(REMOVEFLAG, coordinates, currentGame);

        case RESET:
            return computeAction(RESET, coordinates, currentGame);

        case EXIT:
            return computeAction(EXIT, coordinates, currentGame);

        default:
            break;
    }

    return computeAction(UNKNOWN, coordinates, currentGame);
}




void updateCurrentGame(int feedback[4][4]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            currentGame[i][j] =  feedback[i][j];
        }
    }
}




int main (int argc, char* argv[]) {

    computeArgs(argc, argv);
    initCurrentGame();




    // SOCKETS -----------------------------------------------
    struct sockaddr_storage storage;
    if(addrparse(ipVersion, port, &storage) != 0){
        logexit("addrparse");
    }

    // Inicializar Socket
    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if(s == -1){
        logexit("socket");
    }

    // Inicializar Conexao
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if(connect(s, addr, sizeof(storage)) != 0){
        logexit("connect");
    }





    // LENDO COMANDO ---------------------------------------
    // variavel p/ o comando inserido pelo cliente
    char cmd[1024];

    // Inicio da leitura do que o cliente coloca no terminal
    while(1) {
        // tentativa de acao do cliente
        struct action solicitation;

        // le o nome do comando
        scanf("%s", cmd);
        int commandType = computeCmdType(cmd);
        int coordinates[2]; // cria coordenadas lixo por enquanto

        // chamada especifica de acordo com o tipo de acao do cliente
        if (commandType == UNKNOWN) {
            errorMessage(COMMANDNOTFOUND);
            // tem que continuar esperando um comando valido *********** !!!!!!!!!!!!!!! &*****&**&(*&)
        }

        //cria uma struct action com o comando e as coordenadas passadas pelo cliente (resquested action do client)
        solicitation = computeClientAction(commandType, coordinates);

        if (isAvailable) {
            // enviando pedido de acao para servidor
            int count = send(s, &solicitation, sizeof(solicitation), 0);

            if (commandType == EXIT) {
                close(s);
                break;
            }

            if(count != sizeof(solicitation)){
                logexit("send");
            }

            // recebendo resposta do servidor
            struct action feedback;
            count = recv(s, &feedback, sizeof(feedback), 0);
            updateCurrentGame(feedback.board);

            // verifica se ja ganhou, perdeu ou se so vai atualizar o board
            if(feedback.type == GAMEOVER) {
                printf("GAME OVER\n");
                gameStatus = ENDGAME;
            } else if (feedback.type == WIN) {
                printf("YOU WIN!\n");
                gameStatus = ENDGAME;
            } else { // so atualizou e printa o tabuleiro
                printGame(currentGame);
            }
        }
    }
    return 0;
}