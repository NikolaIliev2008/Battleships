#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define BOARD_SIZE 10
#define MAX_SHIPS 10
#define MAX_MOVES 200
#define REPLAY_DIR "replays"

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

// Структура за записване на ход
typedef struct {
    char player_name[32];
    int row, col;
    int hit;
    int ship_sunk;
    int ship_length;
    char timestamp[30];
} Move;

// Структура за записване на цялата игра
typedef struct {
    Player player1_initial;
    Player player2_initial;
    Move moves[MAX_MOVES];
    int move_count;
    char winner[32];
    char start_time[30];
    char end_time[30];
} GameReplay;

Player player1, player2;
GameReplay current_replay;

// Функционални декларации
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

// Нови функции за риплей
void get_current_time(char* buffer);
void init_replay();
void add_move_to_replay(const char* player_name, int row, int col, int hit, int ship_sunk, int ship_length);
void save_replay(const char* password);
void load_and_play_replay();
void simple_encrypt(char* data, int length, const char* password);
void replay_menu();

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
    printf("1. Play New Game\n");
    printf("2. Watch Replay\n");
    printf("Choose option: ");
    
    int choice;
    scanf("%d", &choice);
    
    if(choice == 2) {
        replay_menu();
        return 0;
    }
    
    // Инициализиране на риплей системата
    init_replay();
    
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
    
    // Запазване на началните състояния
    memcpy(&current_replay.player1_initial, &player1, sizeof(Player));
    memcpy(&current_replay.player2_initial, &player2, sizeof(Player));
    
    printf("\nPress Enter to start the game...");
    getchar();
    clear_screen();
    
    play_game();
    
    // Запазване на риплей след края на играта
    printf("\nDo you want to save the game replay? (y/n): ");
    char save_choice;
    scanf(" %c", &save_choice);
    
    if(save_choice == 'y' || save_choice == 'Y') {
        printf("Enter password for encryption (or press Enter for no password): ");
        char password[100];
        getchar(); // consume newline
        fgets(password, sizeof(password), stdin);
        password[strcspn(password, "\n")] = 0; // remove newline
        
        save_replay(strlen(password) > 0 ? password : NULL);
    }
    
    return 0;
}

void get_current_time(char* buffer) {
    time_t rawtime;
    struct tm* timeinfo;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    strftime(buffer, 30, "%Y-%m-%d %H:%M:%S", timeinfo);
}

void init_replay() {
    memset(&current_replay, 0, sizeof(GameReplay));
    get_current_time(current_replay.start_time);
    current_replay.move_count = 0;
}

void add_move_to_replay(const char* player_name, int row, int col, int hit, int ship_sunk, int ship_length) {
    if(current_replay.move_count >= MAX_MOVES) return;
    
    Move* move = &current_replay.moves[current_replay.move_count];
    strcpy(move->player_name, player_name);
    move->row = row;
    move->col = col;
    move->hit = hit;
    move->ship_sunk = ship_sunk;
    move->ship_length = ship_length;
    get_current_time(move->timestamp);
    
    current_replay.move_count++;
}

void simple_encrypt(char* data, int length, const char* password) {
    if(!password || strlen(password) == 0) return;
    
    int password_len = strlen(password);
    for(int i = 0; i < length; i++) {
        data[i] ^= password[i % password_len];
    }
}

void save_replay(const char* password) {
    // Създаване на директория за записи
    #ifdef _WIN32
        system("mkdir replays 2>nul");
    #else
        system("mkdir -p replays");
    #endif
    
    // Генериране на име на файл с времева отметка
    char filename[100];
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    strftime(filename, sizeof(filename), "replays/game_%Y%m%d_%H%M%S.replay", timeinfo);
    
    // Запазване на време на край
    get_current_time(current_replay.end_time);
    
    FILE* file = fopen(filename, "wb");
    if(!file) {
        printf("Error saving replay!\n");
        return;
    }
    
    // Криптиране ако е зададена парола
    char* data = (char*)&current_replay;
    int data_size = sizeof(GameReplay);
    
    if(password && strlen(password) > 0) {
        simple_encrypt(data, data_size, password);
    }
    
    fwrite(&current_replay, sizeof(GameReplay), 1, file);
    fclose(file);
    
    // Възстановяване на данните след криптиране
    if(password && strlen(password) > 0) {
        simple_encrypt(data, data_size, password);
    }
    
    printf("Game replay saved as: %s\n", filename);
}

void load_and_play_replay() {
    char filename[100];
    char password[100];
    
    printf("Enter replay filename: ");
    scanf("%s", filename);
    
    printf("Enter password (or press Enter if no password): ");
    getchar(); // consume newline
    fgets(password, sizeof(password), stdin);
    password[strcspn(password, "\n")] = 0;
    
    FILE* file = fopen(filename, "rb");
    if(!file) {
        printf("Could not open replay file!\n");
        return;
    }
    
    GameReplay replay;
    fread(&replay, sizeof(GameReplay), 1, file);
    fclose(file);
    
    // Декриптиране ако е зададена парола
    if(strlen(password) > 0) {
        simple_encrypt((char*)&replay, sizeof(GameReplay), password);
    }
    
    // Показване на информация за играта
    printf("\n=== GAME REPLAY INFO ===\n");
    printf("Player 1: %s\n", replay.player1_initial.name);
    printf("Player 2: %s\n", replay.player2_initial.name);
    printf("Start time: %s\n", replay.start_time);
    printf("End time: %s\n", replay.end_time);
    printf("Winner: %s\n", replay.winner);
    printf("Total moves: %d\n", replay.move_count);
    printf("========================\n\n");
    
    printf("Press Enter to start replay...");
    getchar();
    
    // Възстановяване на началните състояния
    Player p1 = replay.player1_initial;
    Player p2 = replay.player2_initial;
    
    // Изчистване на атаките за визуализация на риплея
    for(int i = 0; i < BOARD_SIZE; i++) {
        for(int j = 0; j < BOARD_SIZE; j++) {
            p1.attacks[i][j] = EMPTY;
            p2.attacks[i][j] = EMPTY;
            if(p1.board[i][j] == HIT) p1.board[i][j] = SHIP;
            if(p2.board[i][j] == HIT) p2.board[i][j] = SHIP;
        }
    }
    
    // Възпроизвеждане на всеки ход
    for(int i = 0; i < replay.move_count; i++) {
        clear_screen();
        Move* move = &replay.moves[i];
        
        printf("=== MOVE %d ===\n", i + 1);
        printf("Player: %s\n", move->player_name);
        printf("Target: %c%d\n", row_to_coord(move->row), move->col + 1);
        printf("Time: %s\n", move->timestamp);
        
        // Определяне на атакуващ и защитаващ се играч
        Player* attacker = (strcmp(move->player_name, p1.name) == 0) ? &p1 : &p2;
        Player* defender = (strcmp(move->player_name, p1.name) == 0) ? &p2 : &p1;
        
        // Прилагане на хода
        if(move->hit) {
            attacker->attacks[move->row][move->col] = HIT;
            defender->board[move->row][move->col] = HIT;
            printf("Result: HIT!\n");
            
            if(move->ship_sunk) {
                printf("SHIP SUNK! (Length: %d)\n", move->ship_length);
            }
        } else {
            attacker->attacks[move->row][move->col] = MISS;
            printf("Result: MISS!\n");
        }
        
        // Показване на дъската на атакуващия
        printf("\n%s's attack board:\n", move->player_name);
        print_board(attacker->attacks, 0);
        
        printf("\nPress Enter for next move...");
        getchar();
    }
    
    printf("\n=== GAME OVER ===\n");
    printf("Winner: %s\n", replay.winner);
    
    printf("\nFinal boards:\n");
    printf("\n%s's board:\n", replay.player1_initial.name);
    print_board(replay.player1_initial.board, 1);
    printf("\n%s's board:\n", replay.player2_initial.name);
    print_board(replay.player2_initial.board, 1);
}

void replay_menu() {
    while(1) {
        printf("\n=== REPLAY MENU ===\n");
        printf("1. List available replays\n");
        printf("2. Watch replay\n");
        printf("3. Return to main menu\n");
        printf("Choose option: ");
        
        int choice;
        scanf("%d", &choice);
        
        switch(choice) {
            case 1:
                printf("\nAvailable replays:\n");
                #ifdef _WIN32
                    system("dir /b replays\\*.replay 2>nul");
                #else
                    system("ls -1 replays/*.replay 2>/dev/null | xargs -I {} basename {}");
                #endif
                break;
                
            case 2:
                load_and_play_replay();
                break;
                
            case 3:
                return;
                
            default:
                printf("Invalid choice!\n");
        }
    }
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
        strcpy(current_replay.winner, player2.name);
    } else {
        printf("\n %s WINS! \n", player1.name);
        printf("%s sunk all of %s's ships!\n", player1.name, player2.name);
        strcpy(current_replay.winner, player1.name);
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
    
    int ship_sunk = 0;
    int ship_length = 0;
    
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
                    ship_length = ship->length;
                    break;
                }
            }
            
            if(belongs_to_ship && ship->hits == ship->length) {
                ship->sunk = 1;
                ship_sunk = 1;
                defender->ships_sunk++;
                printf("SHIP SUNK! (%d cells)\n", ship->length);
                printf("Ships remaining: %d\n", MAX_SHIPS - defender->ships_sunk);
            }
        }
        
        // Добавяне на хода към риплей
        add_move_to_replay(attacker->name, row, col, 1, ship_sunk, ship_length);
        
        return 1; 
    } else {
        attacker->attacks[row][col] = MISS;
        
        // Добавяне на хода към риплей
        add_move_to_replay(attacker->name, row, col, 0, 0, 0);
        
        return 0;
    }
}

int game_over() {
    return (player1.ships_sunk == MAX_SHIPS || player2.ships_sunk == MAX_SHIPS);
}