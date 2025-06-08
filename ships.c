#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define BOARD_SIZE 10
#define MAX_SHIPS 10

typedef enum {
    EMPTY = 0,
    SHIP = 1,
    HIT = 2,
    MISS = 3
} CellState;

typedef enum {
    UP = 0,
    DOWN = 1,
    LEFT = 2,
    RIGHT = 3
} Direction;

typedef struct {
    int row, col;
    int length;
    Direction direction;
    int hits;
    int sunk;
} Ship;

typedef struct {
    CellState board[BOARD_SIZE][BOARD_SIZE];
    CellState attacks[BOARD_SIZE][BOARD_SIZE];
    Ship ships[MAX_SHIPS];
    int ship_count;
    int ships_sunk;
    char name[32];
} Player;

Player player1, player2;

void clear_screen();
void print_board(CellState board[BOARD_SIZE][BOARD_SIZE], int show_ships);
int coord_to_row(char c);
int coord_to_col(int n);
char row_to_coord(int row);
int is_valid_position(Player* player, int row, int col, int length, Direction dir);
int place_ship(Player* player, int row, int col, int length, Direction dir);
void setup_player_ships(Player* player);
void play_game();
int make_attack(Player* attacker, Player* defender, int row, int col);
int game_over();

int main() {
    memset(&player1, 0, sizeof(Player));
    memset(&player2, 0, sizeof(Player));
    
    for(int i = 0; i < BOARD_SIZE; i++) {
        for(int j = 0; j < BOARD_SIZE; j++) {
            player1.board[i][j] = EMPTY;
            player1.attacks[i][j] = EMPTY;
            player2.board[i][j] = EMPTY;
            player2.attacks[i][j] = EMPTY;
        }
    }
    
    printf("== BATTLESHIP GAME ==\n\n");
    
    printf("Enter Player 1 name: ");
    scanf("%s", player1.name);
    printf("Enter Player 2 name: ");
    scanf("%s", player2.name);
    
    printf("\n%s will place ships first.\n", player1.name);
    setup_player_ships(&player1);
    
    printf("\nPress Enter to continue...");
    getchar();
    getchar();
    clear_screen();
    
    printf("%s will now place ships.\n", player2.name);
    setup_player_ships(&player2);
    
    printf("\nPress Enter to start the game...");
    getchar();
    clear_screen();
    
    play_game();
    
    return 0;
}

void clear_screen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void print_board(CellState board[BOARD_SIZE][BOARD_SIZE], int show_ships) {
    printf("   ");
    for(int i = 1; i <= BOARD_SIZE; i++) {
        printf("%2d ", i);
    }
    printf("\n");
    
    for(int i = 0; i < BOARD_SIZE; i++) {
        printf("%c  ", 'A' + i);
        for(int j = 0; j < BOARD_SIZE; j++) {
            switch(board[i][j]) {
                case EMPTY:
                    printf(" . ");
                    break;
                case SHIP:
                    printf(show_ships ? " # " : " . ");
                    break;
                case HIT:
                    printf(" X ");
                    break;
                case MISS:
                    printf(" O ");
                    break;
            }
        }
        printf("\n");
    }
}

int coord_to_row(char c) {
    return toupper(c) - 'A';
}

int coord_to_col(int n) {
    return n - 1;
}

char row_to_coord(int row) {
    return 'A' + row;
}

int is_valid_position(Player* player, int row, int col, int length, Direction dir) {
    if(row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return 0;
    }
    
    int end_row = row, end_col = col;
    
    switch(dir) {
        case UP:
            end_row = row - length + 1;
            break;
        case DOWN:
            end_row = row + length - 1;
            break;
        case LEFT:
            end_col = col - length + 1;
            break;
        case RIGHT:
            end_col = col + length - 1;
            break;
    }
    
    if(end_row < 0 || end_row >= BOARD_SIZE || end_col < 0 || end_col >= BOARD_SIZE) {
        return 0;
    }
    
    int min_row = (row < end_row) ? row : end_row;
    int max_row = (row > end_row) ? row : end_row;
    int min_col = (col < end_col) ? col : end_col;
    int max_col = (col > end_col) ? col : end_col;
    
    for(int i = min_row - 1; i <= max_row + 1; i++) {
        for(int j = min_col - 1; j <= max_col + 1; j++) {
            if(i >= 0 && i < BOARD_SIZE && j >= 0 && j < BOARD_SIZE) {
                if(player->board[i][j] == SHIP) {
                    return 0; 
                }
            }
        }
    }
    
    return 1;
}

int place_ship(Player* player, int row, int col, int length, Direction dir) {
    if(!is_valid_position(player, row, col, length, dir)) {
        return 0;
    }
    
    Ship* ship = &player->ships[player->ship_count];
    ship->row = row;
    ship->col = col;
    ship->length = length;
    ship->direction = dir;
    ship->hits = 0;
    ship->sunk = 0;
    
    for(int i = 0; i < length; i++) {
        int curr_row = row, curr_col = col;
        
        switch(dir) {
            case UP:
                curr_row = row - i;
                break;
            case DOWN:
                curr_row = row + i;
                break;
            case LEFT:
                curr_col = col - i;
                break;
            case RIGHT:
                curr_col = col + i;
                break;
        }
        
        player->board[curr_row][curr_col] = SHIP;
    }
    
    player->ship_count++;
    return 1;
}

void setup_player_ships(Player* player) {
    int ship_sizes[] = {2, 2, 2, 2, 3, 3, 3, 4, 4, 6};
    char* ship_names[] = {"Small", "Small", "Small", "Small", 
                         "Medium", "Medium", "Medium", 
                         "Large", "Large", "Cruiser"};
    
    printf("\n== %s's Ship Placement ==\n", player->name);
    printf("Ship types: 4 Small(2), 3 Medium(3), 2 Large(4), 1 Cruiser(6)\n");
    printf("Directions: 0=UP, 1=DOWN, 2=LEFT, 3=RIGHT\n\n");
    
    for(int i = 0; i < MAX_SHIPS; i++) {
        int placed = 0;
        
        while(!placed) {
            printf("\nCurrent board:\n");
            print_board(player->board, 1);
            
            printf("\nPlace %s ship (length %d) - Ship %d/10\n", 
                   ship_names[i], ship_sizes[i], i + 1);
            printf("Enter position and direction: ");
            
            char pos[10];
            int dir;
            
            if(scanf("%s %d", pos, &dir) != 2) {
                printf("Invalid input! \n");
                while(getchar() != '\n');
                continue;
            }
            
            if(strlen(pos) < 2 || dir < 0 || dir > 3) {
                printf("Invalid input format!\n");
                continue;
            }
            
            int row = coord_to_row(pos[0]);
            int col = coord_to_col(atoi(pos + 1));
            
            if(row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
                printf("Invalid coordinates!\n");
                continue;
            }
            
            if(place_ship(player, row, col, ship_sizes[i], (Direction)dir)) {
                printf("Ship placed successfully!\n");
                placed = 1;
            } else {
                printf("Invalid position! Ships cannot touch.\n");
            }
        }
    }
    
    printf("\nAll ships placed for %s!\n", player->name);
    print_board(player->board, 1);
}

void play_game() {
    Player* current = &player1;
    Player* opponent = &player2;
    
    printf("\n== GAME START ==\n");
    
    while(!game_over()) {
        printf("\n--- %s's Turn ---\n", current->name);
        printf("Your attack board:\n");
        print_board(current->attacks, 0);
        
        printf("\nEnter target coordinates: ");
        char pos[10];
        scanf("%s", pos);
        
        if(strlen(pos) < 2) {
            printf("Invalid input.. Try again.\n");
            continue;
        }
        
        int row = coord_to_row(pos[0]);
        int col = coord_to_col(atoi(pos + 1));
        
        if(row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
            printf("Invalid coordinates!\n");
            continue;
        }
        
        int result = make_attack(current, opponent, row, col);
        
        if(result == -1) {
            printf("You already attacked this position!\n");
            continue;
        } else if(result == 1) {
            printf("HIT! \n");
        } else {
            printf("MISS! \n");
            Player* temp = current;
            current = opponent;
            opponent = temp;
            
            printf("\nPress Enter to continue...");
            getchar();
            getchar();
            clear_screen();
        }
    }
    
    if(player1.ships_sunk == MAX_SHIPS) {
        printf("\n %s WINS! \n", player2.name);
        printf("%s sunk all of %s's ships!\n", player2.name, player1.name);
    } else {
        printf("\n %s WINS! \n", player1.name);
        printf("%s sunk all of %s's ships!\n", player1.name, player2.name);
    }
    
    printf("\nFinal boards:\n");
    printf("\n%s's board:\n", player1.name);
    print_board(player1.board, 1);
    printf("\n%s's board:\n", player2.name);
    print_board(player2.board, 1);
}

int make_attack(Player* attacker, Player* defender, int row, int col) {
    if(attacker->attacks[row][col] != EMPTY) {
        return -1; 
    }
    
    if(defender->board[row][col] == SHIP) {
        attacker->attacks[row][col] = HIT;
        defender->board[row][col] = HIT;
        
        for(int i = 0; i < defender->ship_count; i++) {
            Ship* ship = &defender->ships[i];
            if(ship->sunk){
                continue;
            }
            int belongs_to_ship = 0;
            for(int j = 0; j < ship->length; j++) {
                int ship_row = ship->row, ship_col = ship->col;
                
                switch(ship->direction) {
                    case UP: 
                        ship_row = ship->row - j; 
                        break;    
                    case DOWN: 
                        ship_row = ship->row + j;
                        break; 
                    case LEFT: 
                        ship_col = ship->col - j;
                        break;    
                    case RIGHT: 
                        ship_col = ship->col + j;
                        break;
                }
                
                if(ship_row == row && ship_col == col) {
                    ship->hits++;
                    belongs_to_ship = 1;
                    break;
                }
            }
            
            if(belongs_to_ship && ship->hits == ship->length) {
                ship->sunk = 1;
                defender->ships_sunk++;
                printf("SHIP SUNK! (%d cells)\n", ship->length);
                printf("Ships remaining: %d\n", MAX_SHIPS - defender->ships_sunk);
            }
        }
        
        return 1; 
    } else {
        attacker->attacks[row][col] = MISS;
        return 0;
    }
}

int game_over() {
    return (player1.ships_sunk == MAX_SHIPS || player2.ships_sunk == MAX_SHIPS);
}