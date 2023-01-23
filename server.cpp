#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<arpa/inet.h>
#include<fcntl.h>
#include<unistd.h>
#include<pthread.h>
#include<semaphore.h>
#include<iostream>
#include<vector>

#define PORT 1101

using namespace std;

sem_t sem1, sem2;
char client_message[2000];
char buffer[1024];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

class Sudoku {

    public:
        int board[9][9];

        Sudoku(){
            generateSudoku(1);
        }

        void printBoard() {
            for(int i = 0; i < 9; i++) {
                for(int j = 0; j < 9; j++) {
                    
                if (board[i][j] == 0) {
                    cout << "_" << " ";
                } else {
                    cout << board[i][j] << " ";
                }
                    if(j == 2 || j == 5) {
                        cout << "| ";
                    }
                }
                cout << endl;
                if(i == 2 || i == 5) {
                    cout << "---------------------" << endl;
                }
            }
        }

        bool isSolved() {
            // check rows
            for(int i = 0; i < 9; i++) {
                vector<int> check(9);
                for(int j = 0; j < 9; j++) {
                    if(board[i][j] != 0) {
                        check[board[i][j]-1] = 1;
                    }
                }
                for(int j = 0; j < 9; j++) {
                    if(check[j] != 1) {
                        return false;
                    }
                }
            }
            // check columns
            for(int j = 0; j < 9; j++) {
                vector<int> check(9);
                for(int i = 0; i < 9; i++) {
                    if(board[i][j] != 0) {
                        check[board[i][j]-1] = 1;
                    }
                }
                for(int i = 0; i < 9; i++) {
                    if(check[i] != 1) {
                        return false;
                    }
                }
            }
            // check 3x3 grids
            for(int i = 0; i < 9; i += 3) {
                for(int j = 0; j < 9; j += 3) {
                    vector<int> check(9);
                    for(int row = i; row < i+3; row++) {
                        for(int col = j; col < j+3; col++) {
                            if(board[row][col] != 0) {
                                check[board[row][col]-1] = 1;
                            }
                        }
                    }
                    for(int k = 0; k < 9; k++) {
                        if(check[k] != 1) {
                            return false;
                        }
                    }
                }
            }
            return true;
        }

        int addNumber(int row, int col, int num) {
            row = row-1;
            col = col-1;

            if(row < 0 || row > 8 || col < 0 || col > 8) {
                return -1;
            }
            if(num < 1 || num > 9) {
                return -2;
            }
            if(board[row][col] != 0) {
                return -3;
            }
            if(isSafe(row, col, num) == false){
                return -4;
            }
            board[row][col] = num;

            if (isSolved()) {
                return 1;
            }

            return 0;
        }

        string boardToFlat(){
            string flatBoard = "sudoku";
            int k = 0;
            for (int i = 0; i < 9; i++) {
                for (int j = 0; j < 9; j++){
                    flatBoard += to_string(board[i][j]);
                }
            }
            return flatBoard;
        }

        void flatToBoard(string flatBoard){
            int k = 6;
            for (int i = 0; i < 9; i++) {
                for (int j = 0; j < 9; j++){
                    board[i][j] = flatBoard[k++]-48;
                }
            }
        }
    private:

        bool fillBoard(int row, int col) {
            // check if we have reached 8th column
            if (col == 9) {
                col = 0;
                row++;
            }
            // check if we have reached 8th row
            if (row == 9) {
                return true;
            }
            // check if the current position already contains value
            if (board[row][col] != 0) {
                return fillBoard(row, col + 1);
            }
            // fill the board
            for (int num = 1; num <= 9; num++) {
                if (isSafe(row, col, num)) {
                    board[row][col] = num;
                    if (fillBoard(row, col + 1)) {
                        return true;
                    }
                    board[row][col] = 0;
                }
            }
            return false;
        }

        bool isSafe(int row, int col, int num) {
            // check in the same row
            for (int x = 0; x < 9; x++) {
                if (board[row][x] == num) {
                    return false;
                }
            }
            // check in the same column
            for (int x = 0; x < 9; x++) {
                if (board[x][col] == num) {
                    return false;
                }
            }
            // check in the same 3x3 grid
            int startRow = (row / 3) * 3;
            int startCol = (col / 3) * 3;
            for (int i = 0; i < 3; i++) {
                for (int j = 0; j < 3; j++) {
                    if (board[i + startRow][j + startCol] == num) {
                        return false;
                    }
                }
            }
            return true;
        }

        void generateSudoku(int difficulty) {
            // Initialize board with all zeroes
            for(int i = 0; i < 9; i++) {
                for(int j = 0; j < 9; j++) {
                    board[i][j] = 0;
                }
            }
            fillBoard(0, 0);
            // remove some values from board based on difficulty
            int totalCells = 9*9;
            int cellsToRemove = totalCells*(difficulty/10.0);
            while(cellsToRemove > 0) {
                int randomRow = rand() % 9;
                int randomCol = rand() % 9;
                if(board[randomRow][randomCol] != 0) {
                    board[randomRow][randomCol] = 0;
                    cellsToRemove--;
                }
            }
        }

};

class playerPair {
public:
    pthread_t player1;
    pthread_t player2;
    Sudoku sudoku;
    int sockets[2];
    int gameNumber = 0;
    int playerTurn = 0;
    int pcount = 0;
};

int currPair = 0;
playerPair pairs[1000];

void SendMessage(int gameId, int socket, string message){
    char *msg = (char*)malloc(sizeof(char)*87);
    strcpy(msg, message.c_str());
    send(pairs[gameId].sockets[socket], msg, sizeof(char)*87,0);
    memset(&msg, 0, sizeof(msg));
}

void *player1(void* arg)
{
    int gameId = currPair;
    int player1id = 0;
    int player2id = 1;
    int n;

    cout << "[G-" << gameId << "]: Player 1 joined game." << endl;

    SendMessage(gameId, player1id, pairs[gameId].sudoku.boardToFlat());

    while(true){

        if(pairs[gameId].playerTurn == 1)
        {
            n = recv(pairs[gameId].sockets[0], client_message, 2000, 0);
            if(strstr(client_message, "leave") != NULL || n < 1) 
            {
                break;
            }
            
            usleep(100);

            string cMsgStr(client_message);
            if(cMsgStr.size() != 3) {
                usleep(100);
                SendMessage(gameId, player1id, "wmove");
                memset(&client_message, 0, sizeof (client_message));
                cout << "[G-" << gameId << "]: Player 1 wrong input." << endl;
            } else {
                int row = cMsgStr[0]-48;
                int col = cMsgStr[1]-48;
                int num = cMsgStr[2]-48;

                int response = pairs[gameId].sudoku.addNumber(row, col, num);

                //zly ruch
                if (response < 0 ) {
                    SendMessage(gameId, player1id, "wmove");
                    memset(&client_message, 0, sizeof (client_message));
                    cout << "[G-" << gameId << "]: Player 1 wrong move." << endl;
                }

                //dobre drugi gracz gra
                if (response == 0) {
                    usleep(100);
                    SendMessage(gameId, player1id, pairs[gameId].sudoku.boardToFlat());
                    pairs[gameId].playerTurn = 2;
                    usleep(100);
                    SendMessage(gameId, player2id, pairs[gameId].sudoku.boardToFlat());
                    usleep(100);
                    SendMessage(gameId, player2id, "proceed");
                    cout << "[G-" << gameId << "]: Player 1 correct input." << endl;
                }

                // sudoku rozwiazane
                if (response == 1){
                    SendMessage(gameId, player1id, "ggend");
                    SendMessage(gameId, player2id, "ggend");
                }

            }
        }
    }

    pairs[gameId].playerTurn = 2;
    SendMessage(gameId, player2id, "leave");

    close(pairs[gameId].sockets[0]);
    close(pairs[gameId].sockets[1]);

    cout << "[G-" << gameId << "]: Player 1 left." << endl;
    pthread_exit(NULL);
}

void *player2(void* arg)
{
    int gameId = currPair;
    cout << "[G-" << gameId << "]: Player 2 joined game." << endl;
    int n;
    int player1id = 0;
    int player2id = 1;

    SendMessage(gameId, player2id, pairs[gameId].sudoku.boardToFlat());

    sleep(1);
    pairs[gameId].playerTurn = 1;

    SendMessage(gameId, player1id, "proceed");

    for(;;)
    {
        if(pairs[gameId].playerTurn == 2)
        {
            n = recv(pairs[gameId].sockets[1], client_message, 2000, 0);
            if(strstr(client_message, "leave") != NULL || n < 1) 
            {
                break;
            }
            
            usleep(100);

            string cMsgStr(client_message);
            if(cMsgStr.size() != 3) {
                usleep(100);
                SendMessage(gameId, player2id, "wmove");
                memset(&client_message, 0, sizeof (client_message));
                cout << "[G-" << gameId << "]: Player 2 wrong input." << endl;
            } else {
                int row = cMsgStr[0]-48;
                int col = cMsgStr[1]-48;
                int num = cMsgStr[2]-48;

                int response = pairs[gameId].sudoku.addNumber(row, col, num);

                //zly ruch
                if (response < 0 ) {
                    SendMessage(gameId, player2id, "wmove");
                    memset(&client_message, 0, sizeof (client_message));
                    cout << "[G-" << gameId << "]: Player 2 wrong move." << endl;
                }

                //dobre drugi gracz gra
                if (response == 0) {
                    usleep(100);
                    SendMessage(gameId, player1id, pairs[gameId].sudoku.boardToFlat());
                    usleep(100);
                    SendMessage(gameId, player2id, pairs[gameId].sudoku.boardToFlat());
                    pairs[gameId].playerTurn = 1;
                    usleep(100);
                    SendMessage(gameId, player1id, "proceed");
                    cout << "[G-" << gameId << "]: Player 2 correct input." << endl;

                }

                // sudoku rozwiazane
                if (response == 1){
                    SendMessage(gameId, player1id, "ggend");
                    usleep(100);
                    SendMessage(gameId, player2id, "ggend");
                }

            }

        }
    }

    pairs[gameId].playerTurn = 1;
    SendMessage(gameId, player1id, "leave");

    close(pairs[gameId].sockets[0]);
    close(pairs[gameId].sockets[1]);
    
    cout << "[G-" << gameId << "]: Player 2 left." << endl;
    pthread_exit(NULL);
}

int main()
{
    int serverSocket, newSocket;
    struct sockaddr_in serverAddr;
    struct sockaddr_storage serverStorage;
    socklen_t addr_size;
    int acceptedCycle = 0;

    serverSocket = socket(PF_INET, SOCK_STREAM, 0);

    serverAddr.sin_family = AF_INET;

    serverAddr.sin_port = htons(PORT);

    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);

    memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));

    bind(serverSocket, (struct sockaddr *) &serverAddr, sizeof(serverAddr));

    if(listen(serverSocket,50)==0)
        printf("Server started.\n");
    else
        printf("Error\n");

    pthread_t thread_id;

    while(1)
    {
        addr_size = sizeof(serverStorage);
        usleep(200);

        if(acceptedCycle == 2)
        {
            currPair++;
            acceptedCycle = 0;
        }

        pairs[currPair].sockets[pairs[currPair].pcount] = accept(serverSocket, (struct sockaddr*)&serverStorage, &addr_size);
        
        acceptedCycle++;

        if(pairs[currPair].pcount == 0)
        {
            pairs[currPair].gameNumber = currPair;

            if(pthread_create(&pairs[currPair].player1, NULL, player1, &pairs[currPair].sockets[pairs[currPair].pcount]) != 0)
                printf("Failed to create player 1 thread\n");
            else
            {
                pthread_detach(pairs[currPair].player1);
                pairs[currPair].pcount = 1;
            }
        }
        else
        if(pairs[currPair].pcount == 1)
        {
            pairs[currPair].gameNumber = currPair;

            if(pthread_create(&pairs[currPair].player2, NULL, player2, &pairs[currPair].sockets[pairs[currPair].pcount]) != 0)
                printf("Failed to create player 2 thread\n");
            else
            {
                pthread_detach(pairs[currPair].player2);
                pairs[currPair].pcount = 2;
            }
        }
    }

    close(serverSocket);
    return 0;
}