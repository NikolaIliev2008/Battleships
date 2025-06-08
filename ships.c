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
    int is_ai;
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

// AI структура за интелигентни атаки
typedef struct {
    int hunting;  // дали AI е в режим на търсене на кораб
    int hunt_row, hunt_col;
    int hunt_direction;  // посока за търсене: 0=UP, 1=DOWN, 2=LEFT, 3=RIGHT
    int hunt_hits[10][2]; // позиции на попаденията
    int hunt_hit_count;
} AIState;

Player player1, player2;
GameReplay current_replay;
AIState ai_state;
int last_row = -1, last_col = -1; // за относителни атаки

// Функционални декларации
void clear_screen();
void print_board(CellState board[BOARD_SIZE][BOARD_SIZE], int show_ships);
void print_attacks_with_ships_found(Player* attacker, Player* defender);
int coord_to_row(char c);
int coord_to_col(int n);
char row_to_coord(int row);
int is_valid_position(Player* player, int row, int col, int length, Direction dir);
int place_ship(Player* player, int row, int col, int length, Direction dir);
void setup_player_ships(Player* player);
void setup_player_ships_enhanced(Player* player);
int load_ships_from_file(Player* player, const char* filename);
int save_ships_to_file(Player* player, const char* filename);
void edit_ship_position(Player* player, int ship_index);
void review_current_board(Player* player);
void play_game();
void play_single_player();
int make_attack(Player* attacker, Player* defender, int row, int col);
void ai_make_move(Player* ai_player, Player* human_player);
int get_attack_coordinates(Player* current_player, int* row, int* col);
int game_over();

// Нови функции за риплей
void get_current_time(char* buffer);
void init_replay();
void add_move_to_replay(const char* player_name, int row, int col, int hit, int ship_sunk, int ship_length);
void save_replay(void);
void load_and_play_replay();

void replay_menu();

int main() {
    srand(time(NULL)); // инициализация на random generator
    
    memset(&player1, 0, sizeof(Player));
    memset(&player2, 0, sizeof(Player));
    memset(&ai_state, 0, sizeof(AIState));
    
    for(int i = 0; i < BOARD_SIZE; i++) {
        for(int j = 0; j < BOARD_SIZE; j++) {
            player1.board[i][j] = EMPTY;
            player1.attacks[i][j] = EMPTY;
            player2.board[i][j] = EMPTY;
            player2.attacks[i][j] = EMPTY;
        }
    }
    
    printf("=== ИГРА БОЙНИ КОРАБИ ===\n\n");
    printf("1. Игра с двама играчи\n");
    printf("2. Игра срещу компютър\n");
    printf("3. Преглед на запис\n");
    printf("Изберете опция: ");
    
    int choice;
    scanf("%d", &choice);
    
    if(choice == 3) {
        replay_menu();
        return 0;
    }
    
    // Инициализиране на риплей системата
    init_replay();
    
    if(choice == 2) {
        // Single player mode
        printf("Въведете вашето име: ");
        scanf("%s", player1.name);
        strcpy(player2.name, "Компютър");
        player2.is_ai = 1;
        
        printf("\n%s ще разположи корабите си първо.\n", player1.name);
        setup_player_ships_enhanced(&player1);
        
        printf("\nКомпютърът располага корабите си...\n");
        // AI randomly places ships
        for(int attempt = 0; attempt < 1000; attempt++) {
            memset(&player2, 0, sizeof(Player));
            strcpy(player2.name, "Компютър");
            player2.is_ai = 1;
            for(int i = 0; i < BOARD_SIZE; i++) {
                for(int j = 0; j < BOARD_SIZE; j++) {
                    player2.board[i][j] = EMPTY;
                    player2.attacks[i][j] = EMPTY;
                }
            }
            
            int ship_sizes[] = {2, 2, 2, 2, 3, 3, 3, 4, 4, 6};
            int all_placed = 1;
            
            for(int i = 0; i < MAX_SHIPS; i++) {
                int placed = 0;
                for(int tries = 0; tries < 100; tries++) {
                    int row = rand() % BOARD_SIZE;
                    int col = rand() % BOARD_SIZE;
                    Direction dir = rand() % 4;
                    
                    if(place_ship(&player2, row, col, ship_sizes[i], dir)) {
                        placed = 1;
                        break;
                    }
                }
                if(!placed) {
                    all_placed = 0;
                    break;
                }
            }
            
            if(all_placed) break;
        }
        
        play_single_player();
    } else {
        // Two player mode
        printf("Въведете име на Играч 1: ");
        scanf("%s", player1.name);
        printf("Въведете име на Играч 2: ");
        scanf("%s", player2.name);
        
        printf("\n%s ще разположи корабите си първо.\n", player1.name);
        setup_player_ships_enhanced(&player1);
        
        printf("\nНатиснете Enter за да продължите...");
        getchar();
        getchar();
        clear_screen();
        
        printf("%s сега ще разположи корабите си.\n", player2.name);
        setup_player_ships_enhanced(&player2);
        
        play_game();
    }
    
    // Запазване на риплей след края на играта
    printf("\nИскате ли да запазите записа на играта? (y/n): ");
    char save_choice;
    scanf(" %c", &save_choice);
    
    if(save_choice == 'y' || save_choice == 'Y') {
        save_replay();
    }
    
    return 0;
}

int load_ships_from_file(Player* player, const char* filename) {
    FILE* file = fopen(filename, "r");
    if(!file) {
        printf("Грешка при отваряне на файла!\n");
        return 0;
    }
    
    // Изчистване на дъската
    for(int i = 0; i < BOARD_SIZE; i++) {
        for(int j = 0; j < BOARD_SIZE; j++) {
            player->board[i][j] = EMPTY;
        }
    }
    player->ship_count = 0;
    
    int ship_sizes[] = {2, 2, 2, 2, 3, 3, 3, 4, 4, 6};
    
    char line[100];
    int ships_loaded = 0;
    
    while(fgets(line, sizeof(line), file) && ships_loaded < MAX_SHIPS) {
        char pos[10];
        int dir;
        
        if(sscanf(line, "%s %d", pos, &dir) == 2) {
            if(strlen(pos) < 2 || dir < 0 || dir > 3) {
                printf("Невалиден формат в реда: %s", line);
                continue;
            }
            
            int row = coord_to_row(pos[0]);
            int col = coord_to_col(atoi(pos + 1));
            
            if(row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
                printf("Невалидни координати в реда: %s", line);
                continue;
            }
            
            // Използваме размера за текущия кораб
            if(place_ship(player, row, col, ship_sizes[ships_loaded], (Direction)dir)) {
                ships_loaded++;
            } else {
                printf("Невъзможно поставяне на кораб в позиция %s посока %d\n", pos, dir);
                fclose(file);
                return 0;
            }
        }
    }
    
    fclose(file);
    
    if(ships_loaded == MAX_SHIPS) {
        printf("Успешно заредени %d кораба от файла!\n", ships_loaded);
        return 1;
    } else {
        printf("Зареждането неуспешно - очаквани %d кораба, заредени %d\n", MAX_SHIPS, ships_loaded);
        return 0;
    }
}

int save_ships_to_file(Player* player, const char* filename) {
    FILE* file = fopen(filename, "w");
    if(!file) {
        printf("Грешка при създаване на файла!\n");
        return 0;
    }
    
    for(int i = 0; i < player->ship_count; i++) {
        Ship* ship = &player->ships[i];
        fprintf(file, "%c%d %d\n", row_to_coord(ship->row), ship->col + 1, ship->direction);
    }
    
    fclose(file);
    printf("Корабите са запазени във файла %s\n", filename);
    return 1;
}

void edit_ship_position(Player* player, int ship_index) {
    if(ship_index < 0 || ship_index >= player->ship_count) {
        printf("Невалиден номер на кораб!\n");
        return;
    }
    
    Ship* ship = &player->ships[ship_index];
    int old_length = ship->length;
    
    // Премахване на стария кораб от дъската
    for(int i = 0; i < ship->length; i++) {
        int curr_row = ship->row, curr_col = ship->col;
        
        switch(ship->direction) {
            case UP: curr_row = ship->row - i; break;
            case DOWN: curr_row = ship->row + i; break;
            case LEFT: curr_col = ship->col - i; break;
            case RIGHT: curr_col = ship->col + i; break;
        }
        
        player->board[curr_row][curr_col] = EMPTY;
    }
    
    // Премахване на кораба от масива
    for(int i = ship_index; i < player->ship_count - 1; i++) {
        player->ships[i] = player->ships[i + 1];
    }
    player->ship_count--;
    
    printf("\nРедактиране на кораб с дължина %d:\n", old_length);
    printf("Въведете нова позиция и посока: ");
    
    char pos[10];
    int dir;
    
    if(scanf("%s %d", pos, &dir) != 2 || strlen(pos) < 2 || dir < 0 || dir > 3) {
        printf("Невалиден вход!\n");
        return;
    }
    
    int row = coord_to_row(pos[0]);
    int col = coord_to_col(atoi(pos + 1));
    
    if(row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        printf("Невалидни координати!\n");
        return;
    }
    
    if(place_ship(player, row, col, old_length, (Direction)dir)) {
        printf("Корабът е редактиран успешно!\n");
    } else {
        printf("Невалидна позиция за кораба!\n");
    }
}

void review_current_board(Player* player) {
    printf("\n=== Текуща дъска на %s ===\n", player->name);
    print_board(player->board, 1);
    printf("\nПоставени кораби: %d/10\n", player->ship_count);
    
    if(player->ship_count > 0) {
        printf("\nСписък с кораби:\n");
        char* ship_names[] = {"Малък", "Малък", "Малък", "Малък", 
                             "Среден", "Среден", "Среден", 
                             "Голям", "Голям", "Крайцер"};
        
        for(int i = 0; i < player->ship_count; i++) {
            Ship* ship = &player->ships[i];
            char* dir_names[] = {"НАГОРЕ", "НАДОЛУ", "НАЛЯВО", "НАДЯСНО"};
            printf("%d. %s (%d клетки) - %c%d %s\n", 
                   i + 1, ship_names[i], ship->length,
                   row_to_coord(ship->row), ship->col + 1, dir_names[ship->direction]);
        }
    }
}

void setup_player_ships_enhanced(Player* player) {
    printf("\n=== Разположение на кораби - %s ===\n", player->name);
    printf("Типове кораби: 4 Малки(2), 3 Средни(3), 2 Големи(4), 1 Крайцер(6)\n");
    printf("Посоки: 0=НАГОРЕ, 1=НАДОЛУ, 2=НАЛЯВО, 3=НАДЯСНО\n\n");
    
    // Опция за зареждане от файл
    printf("Искате ли да заредите конфигурация от файл? (y/n): ");
    char load_choice;
    scanf(" %c", &load_choice);
    
    if(load_choice == 'y' || load_choice == 'Y') {
        printf("Въведете име на файла: ");
        char filename[100];
        scanf("%s", filename);
        
        if(load_ships_from_file(player, filename)) {
            printf("Заредена конфигурация:\n");
            print_board(player->board, 1);
            printf("Запазване завършено!\n");
            return;
        } else {
            printf("Зареждането неуспешно. Ще въвеждате ръчно.\n");
        }
    }
    
    int ship_sizes[] = {2, 2, 2, 2, 3, 3, 3, 4, 4, 6};
    char* ship_names[] = {"Малък", "Малък", "Малък", "Малък", 
                         "Среден", "Среден", "Среден", 
                         "Голям", "Голям", "Крайцер"};
    
    while(player->ship_count < MAX_SHIPS) {
        printf("\n=== ОПЦИИ ===\n");
        printf("1. Постави следващ кораб (%s, дължина %d)\n", 
               ship_names[player->ship_count], ship_sizes[player->ship_count]);
        if(player->ship_count > 0) {
            printf("2. Редактирай позиция на кораб\n");
        }
        printf("3. Прегледай дъската\n");
        if(player->ship_count > 0) {
            printf("4. Запази текущата конфигурация във файл\n");
        }
        printf("Изберете опция: ");
        
        int choice;
        if(scanf("%d", &choice) != 1) {
            printf("Невалиден вход!\n");
            while(getchar() != '\n');
            continue;
        }
        
        switch(choice) {
            case 1: {
                // Постави следващ кораб
                int current_ship = player->ship_count;
                int placed = 0;
                
                while(!placed) {
                    printf("\nТекуща дъска:\n");
                    print_board(player->board, 1);
                    
                    printf("\nПостави %s кораб (дължина %d) - Кораб %d/10\n", 
                           ship_names[current_ship], ship_sizes[current_ship], current_ship + 1);
                    printf("Въведете позиция и посока: ");
                    
                    char pos[10];
                    int dir;
                    
                    if(scanf("%s %d", pos, &dir) != 2) {
                        printf("Невалиден вход!\n");
                        while(getchar() != '\n');
                        continue;
                    }
                    
                    if(strlen(pos) < 2 || dir < 0 || dir > 3) {
                        printf("Невалиден формат на входа!\n");
                        continue;
                    }
                    
                    int row = coord_to_row(pos[0]);
                    int col = coord_to_col(atoi(pos + 1));
                    
                    if(row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
                        printf("Невалидни координати!\n");
                        continue;
                    }
                    
                    if(place_ship(player, row, col, ship_sizes[current_ship], (Direction)dir)) {
                        printf("Корабът е поставен успешно!\n");
                        placed = 1;
                    } else {
                        printf("Невалидна позиция! Корабите не могат да се докосват.\n");
                    }
                }
                break;
            }
            
            case 2:
                if(player->ship_count > 0) {
                    printf("Въведете номер на кораб за редактиране (1-%d): ", player->ship_count);
                    int ship_num;
                    if(scanf("%d", &ship_num) == 1 && ship_num >= 1 && ship_num <= player->ship_count) {
                        edit_ship_position(player, ship_num - 1);
                    } else {
                        printf("Невалиден номер на кораб!\n");
                    }
                } else {
                    printf("Невалидна опция!\n");
                }
                break;
                
            case 3:
                review_current_board(player);
                break;
                
            case 4:
                if(player->ship_count > 0) {
                    printf("Въведете име на файла за запазване: ");
                    char filename[100];
                    scanf("%s", filename);
                    save_ships_to_file(player, filename);
                } else {
                    printf("Невалидна опция!\n");
                }
                break;
                
            default:
                printf("Невалидна опция!\n");
        }
    }
    
    printf("\nВсички кораби са поставени за %s!\n", player->name);
    print_board(player->board, 1);
}

void print_attacks_with_ships_found(Player* attacker, Player* defender) {
    printf("\n=== Вашите атаки и намерени кораби ===\n");
    printf("Дъска с атаки:\n");
    print_board(attacker->attacks, 0);
    
    printf("\nНамерени кораби: %d/10\n", defender->ships_sunk);
    printf("Успешни попадения: ");
    int hit_count = 0;
    for(int i = 0; i < BOARD_SIZE; i++) {
        for(int j = 0; j < BOARD_SIZE; j++) {
            if(attacker->attacks[i][j] == HIT) {
                if(hit_count > 0) printf(", ");
                printf("%c%d", row_to_coord(i), j + 1);
                hit_count++;
            }
        }
    }
    if(hit_count == 0) printf("няма");
    
    printf("\nНеуспешни опити: ");
    int miss_count = 0;
    for(int i = 0; i < BOARD_SIZE; i++) {
        for(int j = 0; j < BOARD_SIZE; j++) {
            if(attacker->attacks[i][j] == MISS) {
                if(miss_count > 0) printf(", ");
                printf("%c%d", row_to_coord(i), j + 1);
                miss_count++;
            }
        }
    }
    if(miss_count == 0) printf("няма");
    printf("\n");
}

int get_attack_coordinates(Player* current_player __attribute__((unused)), int* row, int* col) {
    printf("\n=== ОПЦИИ ЗА АТАКА ===\n");
    printf("1. Посочи конкретни координати (напр. A5)\n");
    if(last_row != -1 && last_col != -1) {
        printf("2. Атакувай спрямо последните координати %c%d\n", 
               row_to_coord(last_row), last_col + 1);
    }
    printf("Изберете опция: ");
    
    int choice;
    if(scanf("%d", &choice) != 1) {
        printf("Невалиден вход!\n");
        return 0;
    }
    
    if(choice == 1) {
        printf("Въведете координати за атака: ");
        char pos[10];
        if(scanf("%s", pos) != 1 || strlen(pos) < 2) {
            printf("Невалиден вход!\n");
            return 0;
        }
        
        *row = coord_to_row(pos[0]);
        *col = coord_to_col(atoi(pos + 1));
        
        if(*row < 0 || *row >= BOARD_SIZE || *col < 0 || *col >= BOARD_SIZE) {
            printf("Невалидни координати!\n");
            return 0;
        }
        
        last_row = *row;
        last_col = *col;
        return 1;
        
    } else if(choice == 2 && last_row != -1 && last_col != -1) {
        printf("От позиция %c%d изберете посока:\n", row_to_coord(last_row), last_col + 1);
        printf("1. Нагоре\n2. Надолу\n3. Наляво\n4. Надясно\n");
        printf("Посока: ");
        
        int direction;
        if(scanf("%d", &direction) != 1 || direction < 1 || direction > 4) {
            printf("Невалидна посока!\n");
            return 0;
        }
        
        *row = last_row;
        *col = last_col;
        
        switch(direction) {
            case 1: (*row)--; break; // нагоре
            case 2: (*row)++; break; // надолу
            case 3: (*col)--; break; // наляво
            case 4: (*col)++; break; // надясно
        }
        
        if(*row < 0 || *row >= BOARD_SIZE || *col < 0 || *col >= BOARD_SIZE) {
            printf("Координатите са извън дъската!\n");
            return 0;
        }
        
        last_row = *row;
        last_col = *col;
        return 1;
    }
    
    printf("Невалидна опция!\n");
    return 0;
}

void ai_make_move(Player* ai_player, Player* human_player) {
    int row, col;
    
    // AI стратегия
    if(!ai_state.hunting) {
        // Случайна атака
        do {
            row = rand() % BOARD_SIZE;
            col = rand() % BOARD_SIZE;
        } while(ai_player->attacks[row][col] != EMPTY);
    } else {
        // Интелигентно търсене след попадение
        int found = 0;
        
        // Опитай в текущата посока
        if(ai_state.hunt_direction != -1) {
            row = ai_state.hunt_row;
            col = ai_state.hunt_col;
            
            switch(ai_state.hunt_direction) {
                case 0: row--; break; // UP
                case 1: row++; break; // DOWN
                case 2: col--; break; // LEFT  
                case 3: col++; break; // RIGHT
            }
            
            if(row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE && 
               ai_player->attacks[row][col] == EMPTY) {
                found = 1;
            }
        }
        
        // Ако текущата посока не работи, опитай нова
        if(!found) {
            for(int dir = 0; dir < 4 && !found; dir++) {
                row = ai_state.hunt_row;
                col = ai_state.hunt_col;
                
                switch(dir) {
                    case 0: row--; break;
                    case 1: row++; break;
                    case 2: col--; break;
                    case 3: col++; break;
                }
                
                if(row >= 0 && row < BOARD_SIZE && col >= 0 && col < BOARD_SIZE && 
                   ai_player->attacks[row][col] == EMPTY) {
                    ai_state.hunt_direction = dir;
                    found = 1;
                }
            }
        }
        
        // Ако нищо не работи, върни се към случайна атака
        if(!found) {
            ai_state.hunting = 0;
            ai_state.hunt_direction = -1;
            do {
                row = rand() % BOARD_SIZE;
                col = rand() % BOARD_SIZE;
            } while(ai_player->attacks[row][col] != EMPTY);
        }
    }
    
    printf("Компютърът атакува %c%d...\n", row_to_coord(row), col + 1);
    
    int result = make_attack(ai_player, human_player, row, col);
    
    if(result == 1) {
        printf("ПОПАДЕНИЕ! Компютърът отново атакува.\n");
        
        // Започни или продължи търсенето
        if(!ai_state.hunting) {
            ai_state.hunting = 1;
            ai_state.hunt_row = row;
            ai_state.hunt_col = col;
            ai_state.hunt_direction = -1;
            ai_state.hunt_hit_count = 1;
            ai_state.hunt_hits[0][0] = row;
            ai_state.hunt_hits[0][1] = col;
        } else {
            // Продължи в същата посока
            ai_state.hunt_row = row;
            ai_state.hunt_col = col;
            if(ai_state.hunt_hit_count < 10) {
                ai_state.hunt_hits[ai_state.hunt_hit_count][0] = row;
                ai_state.hunt_hits[ai_state.hunt_hit_count][1] = col;
                ai_state.hunt_hit_count++;
            }
        }
    } else if(result == 0) {
        printf("ПРОПУСК!\n");
        
        if(ai_state.hunting && ai_state.hunt_direction != -1) {
            // Смени посоката
            ai_state.hunt_direction = -1;
        }
    }
    
    // Провери дали корабът е потопен
    if(result == 1) {
        // Провери дали някой кораб е потопен
        for(int i = 0; i < human_player->ship_count; i++) {
            Ship* ship = &human_player->ships[i];
            if(ship->sunk && ship->hits == ship->length) {
                // Корабът е потопен, спри търсенето
                ai_state.hunting = 0;
                ai_state.hunt_direction = -1;
                ai_state.hunt_hit_count = 0;
                break;
            }
        }
    }
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



void save_replay(void) {
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
        printf("Грешка при запазване на записа!\n");
        return;
    }
    
    fwrite(&current_replay, sizeof(GameReplay), 1, file);
    fclose(file);
    
    printf("Записът на играта е запазен като: %s\n", filename);
}

void load_and_play_replay() {
    char filename[100];
    
    printf("Въведете име на файла с записа: ");
    scanf("%s", filename);
    
    FILE* file = fopen(filename, "rb");
    if(!file) {
        printf("Не може да се отвори файлът с записа!\n");
        return;
    }
    
    GameReplay replay;
    fread(&replay, sizeof(GameReplay), 1, file);
    fclose(file);
    
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
    
    // Запазване на началните състояния
    memcpy(&current_replay.player1_initial, &player1, sizeof(Player));
    memcpy(&current_replay.player2_initial, &player2, sizeof(Player));
    
    printf("\nНатиснете Enter за да започне играта...");
    getchar();
    clear_screen();
    
    printf("\n=== ЗАПОЧВА ИГРАТА ===\n");
    
    while(!game_over()) {
        printf("\n--- Ред на %s ---\n", current->name);
        
        printf("\n=== ОПЦИИ ===\n");
        printf("1. Преглед на атаки и намерени кораби\n");
        printf("2. Направи атака\n");
        printf("Изберете опция: ");
        
        int choice;
        if(scanf("%d", &choice) != 1) {
            printf("Невалиден вход!\n");
            while(getchar() != '\n');
            continue;
        }
        
        if(choice == 1) {
            print_attacks_with_ships_found(current, opponent);
            continue;
        } else if(choice == 2) {
            int row, col;
            
            if(!get_attack_coordinates(current, &row, &col)) {
                continue;
            }
            
            int result = make_attack(current, opponent, row, col);
            
            if(result == -1) {
                printf("Вече сте атакували тази позиция!\n");
                continue;
            } else if(result == 1) {
                printf("ПОПАДЕНИЕ!\n");
            } else {
                printf("ПРОПУСК!\n");
                Player* temp = current;
                current = opponent;
                opponent = temp;
                
                printf("\nНатиснете Enter за да продължите...");
                getchar();
                getchar();
                clear_screen();
            }
        } else {
            printf("Невалидна опция!\n");
        }
    }
    
    if(player1.ships_sunk == MAX_SHIPS) {
        printf("\n=== %s ПЕЧЕЛИ! ===\n", player2.name);
        printf("%s потопи всички кораби на %s!\n", player2.name, player1.name);
        strcpy(current_replay.winner, player2.name);
    } else {
        printf("\n=== %s ПЕЧЕЛИ! ===\n", player1.name);
        printf("%s потопи всички кораби на %s!\n", player1.name, player2.name);
        strcpy(current_replay.winner, player1.name);
    }
    
    get_current_time(current_replay.end_time);
    
    printf("\nФинални дъски:\n");
    printf("\nДъска на %s:\n", player1.name);
    print_board(player1.board, 1);
    printf("\nДъска на %s:\n", player2.name);
    print_board(player2.board, 1);
}

void play_single_player() {
    Player* human = &player1;
    Player* ai = &player2;
    Player* current = human; // човекът започва първи
    
    // Запазване на началните състояния
    memcpy(&current_replay.player1_initial, &player1, sizeof(Player));
    memcpy(&current_replay.player2_initial, &player2, sizeof(Player));
    
    printf("\nНатиснете Enter за да започне играта...");
    getchar();
    getchar();
    clear_screen();
    
    printf("\n=== ЗАПОЧВА ИГРАТА СРЕЩУ КОМПЮТЪР ===\n");
    
    while(!game_over()) {
        if(current == human) {
            printf("\n--- Вашия ред ---\n");
            
            printf("\n=== ОПЦИИ ===\n");
            printf("1. Преглед на атаки и намерени кораби\n");
            printf("2. Направи атака\n");
            printf("Изберете опция: ");
            
            int choice;
            if(scanf("%d", &choice) != 1) {
                printf("Невалиден вход!\n");
                while(getchar() != '\n');
                continue;
            }
            
            if(choice == 1) {
                print_attacks_with_ships_found(human, ai);
                continue;
            } else if(choice == 2) {
                int row, col;
                
                if(!get_attack_coordinates(human, &row, &col)) {
                    continue;
                }
                
                int result = make_attack(human, ai, row, col);
                
                if(result == -1) {
                    printf("Вече сте атакували тази позиция!\n");
                    continue;
                } else if(result == 1) {
                    printf("ПОПАДЕНИЕ!\n");
                } else {
                    printf("ПРОПУСК!\n");
                    current = ai; // ред на компютъра
                    
                    printf("\nНатиснете Enter за да продължите...");
                    getchar();
                    getchar();
                    clear_screen();
                }
            } else {
                printf("Невалидна опция!\n");
            }
        } else {
            // AI ред
            printf("\n--- Ред на компютъра ---\n");
            
            ai_make_move(ai, human);
            
            if(!game_over()) {
                // Проверка дали AI трябва да продължи или да смени реда
                int last_move_hit = 0;
                // Намери последното попадение в последните ходове
                if(current_replay.move_count > 0) {
                    Move* last_move = &current_replay.moves[current_replay.move_count - 1];
                    if(strcmp(last_move->player_name, ai->name) == 0 && last_move->hit) {
                        last_move_hit = 1;
                    }
                }
                
                if(!last_move_hit) {
                    current = human; // ред на човека
                    printf("\nНатиснете Enter за да продължите...");
                    getchar();
                    clear_screen();
                }
            }
        }
    }
    
    if(player1.ships_sunk == MAX_SHIPS) {
        printf("\n=== КОМПЮТЪРЪТ ПЕЧЕЛИ! ===\n");
        printf("Компютърът потопи всички ваши кораби!\n");
        strcpy(current_replay.winner, player2.name);
    } else {
        printf("\n=== ВИЕ ПЕЧЕЛИТЕ! ===\n");
        printf("Потопихте всички кораби на компютъра!\n");
        strcpy(current_replay.winner, player1.name);
    }
    
    get_current_time(current_replay.end_time);
    
    printf("\nФинални дъски:\n");
    printf("\nВашата дъска:\n");
    print_board(player1.board, 1);
    printf("\nДъска на компютъра:\n");
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