#include "common.h"

// Client's current game
int currentGame[4][4];

char *ipVersion = "";
char *port = "";
int gameStatus;
int isAvailable = 1;

// Takes the command line arguments for ip version and port number
void computeArgs(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Usage: <ipVersion> <portNumber>\n");
        return 1;
    } else {
        ipVersion = argv[1];
        port = argv[2];
    }
}


// Initializes all board cells as hidden
void initCurrentGame() {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            currentGame[i][j] =  ISSECRET;
        }
    }
}


// Returns the client's command input type
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
                cmdType = REMOVE_FLAG;
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

// Verifies client's coordinates input range
bool accepted(int coordinates[2]){
    return coordinates[0] < 4 && coordinates[0] >= 0 &&
           coordinates[1] < 4 && coordinates[1] >= 0;
}

// Verifies if a cell has flag
bool isFlagged(int coordinates[2]){
    return currentGame[coordinates[0]][coordinates[1]] == HASFLAG;
}

// Verifies if a cell is already revealed
bool isRevealed(int coordinates[2]){
    return currentGame[coordinates[0]][coordinates[1]] != -2;
}


// Computes client command and returns the structure sent over socket
struct action computeClientAction(int cmdType, int coordinates[2]) {
    char comma;
    isAvailable = 1;

    switch(cmdType){

        case START:
            gameStatus = ONGOING;
            return computeAction(START, coordinates, currentGame);

        case REVEAL:
            scanf("%d%c%d", &coordinates[0], &comma, &coordinates[1]);
            if (!accepted(coordinates)) {
                errorMessage(INVALIDCELL);
                isAvailable = 0;
                return computeAction(ERROR, coordinates, currentGame);

            } else if (isRevealed(coordinates)) {
                errorMessage(ALREADYREVEALED);
                isAvailable = 0;
                return computeAction(ERROR, coordinates, currentGame);
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

        case REMOVE_FLAG:
            scanf("%d%c%d", &coordinates[0], &comma, &coordinates[1]);
            if (!accepted(coordinates)) {
                isAvailable = 0;
                break;
            }
            return computeAction(REMOVE_FLAG, coordinates, currentGame);

        case RESET:
            return computeAction(RESET, coordinates, currentGame);

        case EXIT:
            return computeAction(EXIT, coordinates, currentGame);

        default:
            break;
    }

    return computeAction(UNKNOWN, coordinates, currentGame);
}


// Updates client's board  with the one received by server
void updateCurrentGame(int feedback[4][4]) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            currentGame[i][j] =  feedback[i][j];
        }
    }
}




int main (int argc, char* argv[]) {

    computeArgs(argc, argv);  // Initializes ip version and port number
    initCurrentGame();        // Starts initial board


    // Stores information about the IP version and port
    struct sockaddr_storage storage;
    if(addrparse(ipVersion, port, &storage) != 0){
        logexit("addrparse");
    }

    // Creates socket for TCP communication
    int s;
    s = socket(storage.ss_family, SOCK_STREAM, 0);
    if(s == -1){
        logexit("socket");
    }

    // Creates connection
    struct sockaddr *addr = (struct sockaddr *)(&storage);
    if(connect(s, addr, sizeof(storage)) != 0){
        logexit("connect");
    }

    char cmd[1024];
    // Computes client's input
    while(1) {
        struct action solicitation;                 // Structure sent to the server

        scanf("%s", cmd);
        int commandType = computeCmdType(cmd);
        int coordinates[2];

        if (commandType == UNKNOWN) {
            errorMessage(COMMANDNOTFOUND);          // For invalid command
            continue;
        }

        // Creates the structure with client's request
        solicitation = computeClientAction(commandType, coordinates);

        if (isAvailable) {
            // Send to server
            int count = send(s, &solicitation, sizeof(solicitation), 0);

            if (commandType == EXIT) {              // Closes connection
                close(s);
                break;
            }

            if(count != sizeof(solicitation)){
                logexit("send");
            }

            // This will be the structure received by server
            struct action feedback;
            count = recv(s, &feedback, sizeof(feedback), 0);

            // Verifies server feedback, the options are win, gameover or to continue the game
            if(feedback.type == GAMEOVER) {
                printf("GAME OVER\n");
                gameStatus = ENDGAME;
            } else if (feedback.type == WIN) {
                printf("YOU WIN!\n");
                gameStatus = ENDGAME;
            }
            updateCurrentGame(feedback.board);
            printGame(currentGame);
        }
    }
    return 0;
}