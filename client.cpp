#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

using namespace std;

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
            cout << endl;
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


int main(int argc, char** argv) {

    if(argc <= 1) {
        std::cout << "Provide IP address and port";
        return -1;
    }
    
    char message[1000];
    char buffer[1024];
    int clientSocket;
    struct sockaddr_in serverAddr;
    socklen_t addr_size;
    string msgStr;
    bool notify = false;

    Sudoku sudoku;

    int player = 1;
    std::string response = "";

    int boardResponse = 0;

    char playerSign = 'o';

    clientSocket = socket(PF_INET, SOCK_STREAM, 0);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(atoi(argv[2]));
    serverAddr.sin_addr.s_addr = inet_addr(argv[1]);
    memset(serverAddr.sin_zero, '\0', sizeof(serverAddr.sin_zero));
    addr_size = sizeof(serverAddr);
    connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);

    for(;;) {
        if(recv(clientSocket, buffer, 1024, 0) < 0) {
            if(!notify) cout << "No server connection or read error. Waiting for server..." << endl;
            notify = true;
            connect(clientSocket, (struct sockaddr *) &serverAddr, addr_size);
        } else {

            notify = false;
            if (strstr(buffer, "sudoku") != NULL) {
                sudoku.flatToBoard(buffer);
                sudoku.printBoard();

            } else if(strstr(buffer, "proceed") != NULL || strstr(buffer, "wmove") != NULL) {

                if(strstr(buffer, "wmove") != NULL) {
                    cout << "Provide a correct value ex. (123): ";
                } else {
                    cout << "Your move (row, col, num): ";
                }
                
                cin.clear();
                cin >> message;

                if(strstr(message, "exit") != NULL)
                {
                    printf("Exiting\n");
                    break;
                }

                if(send(clientSocket, message, strlen(message), 0) < 0)
                {
                    printf("Send failed\n");
                }

                memset(&message, 0, sizeof (message));
            } else if(strstr(buffer, "leave") != NULL) {

                printf("The game has ended.\n");
                break;

            } else if(strstr(buffer, "ggend") != NULL) {

                printf("Sudoku solved. Game ends now. \n");
                break;
                
            }
        }
    }

    close(clientSocket);
    return 0;

}