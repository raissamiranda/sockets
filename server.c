#include <stdio.h>
#include <stdbool.h>
#include <string.h>



struct action {
    int type;
    int coordinates[2];
    int board[4][4];
};


char format(int position) {
    switch (position) {
        case -3:
            return '>';
        case -2:
            return '-';
        case -1:
            return '*';
        case 0:
            return '0';
        case 1:
            return '1';
        case 2:
            return '2';
    }
}


void print_matrix(struct action* action) {
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            int pos = action->board[i][j];;
            printf("%c      ", format(pos));
        }
        printf("\n");
    }
}




struct action current_action;


int main(int argc, char *argv[]) {

    if (argc != 5 || strcmp(argv[3], "-i" )!= 0) {
        printf("Usage: <ipVersion> <portNumber> -i <inputFilePath>\n");
        return 1;
    }

    char* ipVersion = argv[1];
    char* portNumber = argv[2];
    char* inputFilePath = argv[4];


    int pos;
    char comma;
    FILE *file;
    file = fopen(inputFilePath, "r");

    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            fscanf (file, "%d%c", &pos, &comma);
            //scanf ("%d%c", &pos, &comma);
            current_action.board[i][j] = pos;
        }
    }

    print_matrix(&current_action);


    return 0;
}




