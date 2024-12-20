#include <ncurses.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>

// Dimensions de la carte
#define MAP_HEIGHT 20
#define MAP_WIDTH 50
#define WALL_COUNT 120 // Nombre total de murs
#define VISION_RADIUS 5 // Rayon de la vision initiale
#define TORCH_VISION_BOOST 3 // Augmentation du rayon de vision par la lampe torche
#define TORCH_DURATION 20 // Durée de la lampe torche en mouvements
#define MAX_MONSTERS 10 // Nombre maximum de monstres

// Symboles
#define PLAYER_SYMBOL 0x1F385 // Père Noël
#define TREASURE_SYMBOL 0x1F4B0
#define WALL_SYMBOL 0x1F384 //sapin de Noël
#define MONSTER_SYMBOL 0x26C4
#define TORCH_SYMBOL 0x1F526 // Lampe torche

// Structure pour stocker les coordonnées

typedef struct {
    int x;
    int y;
} Position;

// Fonction pour calculer la distance entre deux points

int calculerDistance(Position a, Position b) {
    return abs(a.x - b.x) + abs(a.y - b.y); // Distance de Manhattan
}

void afficherTutoriel() {
    clear();
    mvprintw(1, 1, "Bienvenue dans le jeu !");
    mvprintw(3, 1, "Commandes :");
    mvprintw(4, 3, "Fleche Haut   : Monter");
    mvprintw(5, 3, "Fleche Bas    : Descendre");
    mvprintw(6, 3, "Fleche Gauche : Aller a gauche");
    mvprintw(7, 3, "Fleche Droite : Aller a droite");
    mvprintw(8, 3, "Q             : Quitter le jeu");
    mvprintw(10, 1, "Interactions :");
    mvprintw(11, 3, "Tresor (%lc) : Augmente votre score.", TREASURE_SYMBOL);
    mvprintw(12, 3, "Monstres (%lc) : Evitez-les ou vous perdrez !", MONSTER_SYMBOL);
    mvprintw(13, 3, "Lampe torche (%lc) : Augmente temporairement votre vision.", TORCH_SYMBOL);
    mvprintw(14, 1, "Trouver le tresor et evitez les monstres. Bonne chance !");
    mvprintw(16, 1, "Appuyez sur une touche pour commencer...");
    refresh();
    getch(); // Attendre que l'utilisateur appuie sur une touche
    clear();
    refresh();
}


// Fonction pour convertir la distance en échelle de température

const char* distanceEnTemperature(int distance) {
    switch (distance) {
        case 0:
            return "TROUVE !";
        case 1: case 2:
            return "BRULANT";
        case 3: case 4:
            return "CHAUD";
        case 5: case 6:
            return "RECHAUFFE";
        case 7: case 8:
            return "TIEDE";
        case 9: case 10: case 11: case 12:
            return "TEMPERE";
        case 13: case 14: case 15: case 16:
            return "FRAIS";
        case 17: case 18: case 19: case 20:
            return "FROID";
        default:
            return "CONGELE";
    }
}

// Fonction pour vérifier si une position est valide (pas de mur, dans les limites)

int positionValide(Position pos, Position walls[], int wallCount) {
    if (pos.x < 1 || pos.x > MAP_WIDTH || pos.y < 1 || pos.y > MAP_HEIGHT) {
        return 0;
    }
    for (int i = 0; i < wallCount; i++) {
        if (pos.x == walls[i].x && pos.y == walls[i].y) {
            return 0;
        }
    }
    return 1;
}

// Fonction pour afficher la carte avec la zone de vision

void afficherCarte(WINDOW *win, Position player, Position treasure, Position monster[], int monsterCount, Position walls[], int wallCount, Position torch, int torchActive, int visionRadius) {
    werase(win);
    box(win, 0, 0);

    for (int y = 1; y <= MAP_HEIGHT; y++) {
        for (int x = 1; x <= MAP_WIDTH; x++) {
            int isWall = 0;
            for (int i = 0; i < wallCount; i++) {
                if (x == walls[i].x && y == walls[i].y) {
                    isWall = 1;
                    break;
                }
            }

            int displayType = 0;
            int currentVisionRadius = visionRadius;
            if (torchActive) {
                currentVisionRadius += TORCH_VISION_BOOST;
            }

            if (abs(player.x - x) <= currentVisionRadius && abs(player.y - y) <= currentVisionRadius) {
                if (player.x == x && player.y == y) {
                    displayType = 1;
                }

                if (treasure.x == x && treasure.y == y) {
                    displayType = 2;
                }

                for (int i = 0; i < monsterCount; i++) {
                    if (monster[i].x == x && monster[i].y == y) {
                        displayType = 3;
                        break;
                    }
                }

                if (torch.x == x && torch.y == y) {
                    displayType = 4;
                }
            } else {
                displayType = 5;
            }
            wchar_t flam = PLAYER_SYMBOL;
            switch (displayType) {
                case 1:
                    mvwprintw(win, y, x * 2, "%lc", flam);
                    break;
                case 2:
                    mvwprintw(win, y, x * 2, "%lc ", TREASURE_SYMBOL);
                    break;
                case 3:
                    mvwprintw(win, y, x * 2, "%lc ", MONSTER_SYMBOL);
                    break;
                case 4:
                    mvwprintw(win, y, x * 2, "%lc ", TORCH_SYMBOL);
                    break;
                case 5:
                    mvwprintw(win, y, x * 2, ". ");
                    break;
                default:
                    if (isWall) {
                        mvwprintw(win, y, x * 2, "%lc ", WALL_SYMBOL);
                    } else {
                        mvwprintw(win, y, x * 2, ". ");
                    }
                    break;
            }
        }
    }
    wrefresh(win);
}

// Fonction pour déplacer les monstres

void deplacerMonstre(Position *monster, Position player) {
    Position next = *monster;
    if (player.x != monster->x) {
        next.x += (player.x > monster->x) ? 1 : -1;
        *monster = next;
        return;
    }
    next = *monster;
    if (player.y != monster->y) {
        next.y += (player.y > monster->y) ? 1 : -1;
        *monster = next;
    }
}

void reinitialiserLampeTorche(Position *torch, Position walls[], int wallCount, Position player, Position treasure) {
    do {
        torch->x = (rand() % MAP_WIDTH) + 1;
        torch->y = (rand() % MAP_HEIGHT) + 1;
    } while (!positionValide(*torch, walls, wallCount) ||
            (torch->x == player.x && torch->y == player.y) ||
            (torch->x == treasure.x && torch->y == treasure.y));
}

void afficherInfos(WINDOW *info, int distance, int score) {
    werase(info);
    mvwprintw(info, 1, 1, "Distance au tresor : %s", distanceEnTemperature(distance));
    mvwprintw(info, 2, 1, "Score : %d", score);
    box(info, 0, 0);
    wrefresh(info);
}

int main() {
    setlocale(LC_ALL, ""); // mettre la console en compatibilité d'affichage unicode sous linux
    initscr();
    noecho();
    curs_set(FALSE);
    keypad(stdscr, TRUE);

    afficherTutoriel();
    WINDOW *carte = newwin(MAP_HEIGHT + 2, MAP_WIDTH * 2 + 3, 1, 1);
    WINDOW *info = newwin(4, MAP_WIDTH * 2 + 2, MAP_HEIGHT + 6, 1);

    box(carte, 0, 0);

    Position player = {2, 2};
    Position treasure = {0, 0};
    Position monster[MAX_MONSTERS];
    int monsterCount = 1;
    Position walls[WALL_COUNT];
    Position torch = {0, 0};
    int torchActive = 0;
    int visionRadius = VISION_RADIUS;
    int torchTimer = 0;
    int moveCount = 0;

    srand(time(NULL));
    int generatedWalls = 0;
    while (generatedWalls < WALL_COUNT) {
        Position wall = {(rand() % MAP_WIDTH) + 1, (rand() % MAP_HEIGHT) + 1};
        if (abs(wall.x - player.x) > 1 || abs(wall.y - player.y) > 1) {
            walls[generatedWalls++] = wall;
        }
    }

    do {
        treasure.x = (rand() % MAP_WIDTH) + 1;
        treasure.y = (rand() % MAP_HEIGHT) + 1;
    } while (!positionValide(treasure, walls, WALL_COUNT));

    monster[0] = (Position){MAP_WIDTH / 2, MAP_HEIGHT / 2};
    do {
        torch.x = (rand() % MAP_WIDTH) + 1;
        torch.y = (rand() % MAP_HEIGHT) + 1;
    } while (!positionValide(torch, walls, WALL_COUNT));

    int distance = calculerDistance(player, treasure);
    int key;
    int gameOver = 0;
    int score = 0;

    while (!gameOver) {
        afficherCarte(carte, player, treasure, monster, monsterCount, walls, WALL_COUNT, torch, torchActive, visionRadius);
        afficherInfos(info, distance, score);

        wrefresh(carte);

        key = getch();
        Position oldPlayer = player;
        switch (key) {
            case KEY_UP: player.y--;
                break;
            case KEY_DOWN: player.y++;
                break;
            case KEY_LEFT: player.x--;
                break;
            case KEY_RIGHT: player.x++;
                break;
            case 'q': gameOver = 1;
                break;
        }

        if (!positionValide(player, walls, WALL_COUNT)) {
            player = oldPlayer;
        } else {
            moveCount++;
        }

        distance = calculerDistance(player, treasure);
        if (distance == 0) {
            score++;
            do {
                treasure.x = (rand() % MAP_WIDTH) + 1;
                treasure.y = (rand() % MAP_HEIGHT) + 1;
            } while (!positionValide(treasure, walls, WALL_COUNT));

            if (monsterCount < MAX_MONSTERS) {
                do {
                    monster[monsterCount].x = (rand() % MAP_WIDTH) + 1;
                    monster[monsterCount].y = (rand() % MAP_HEIGHT) + 1;
                } while (!positionValide(monster[monsterCount], walls, WALL_COUNT));
                monsterCount++;
            }
        }

        if (player.x == torch.x && player.y == torch.y && !torchActive) {
            torchActive = 1;
            visionRadius += TORCH_VISION_BOOST;
            torchTimer = TORCH_DURATION;
            reinitialiserLampeTorche(&torch, walls, WALL_COUNT, player, treasure);
        }

        if (torchActive && torchTimer > 0) {
            torchTimer--;
            if (torchTimer == 0) {
                torchActive = 0;
                visionRadius -= TORCH_VISION_BOOST;
            }
        }

        if (moveCount >= 2) {
            for (int i = 0; i < monsterCount; i++) {
                deplacerMonstre(&monster[i], player);
            }
            moveCount = 0;
        }

        for (int i = 0; i < monsterCount; i++) {
            if (player.x == monster[i].x && player.y == monster[i].y) {
                mvprintw(MAP_HEIGHT + 10, 1, "Vous avez été mange par un monstre ! Game Over.");
                refresh();
                getch(); // Attendre que l'utilisateur voie le message
                gameOver = 1;
            }
        }

    }

    endwin();
    return 0;
}
