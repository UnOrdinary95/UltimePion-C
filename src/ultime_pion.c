// gcc ultime_pion.c -o ultime_pion && ./ultime_pion -Wall -std=c17
// TODO: delete command
#include <stdio.h>

#define TAILLE 5

#define VIDE "·"
#define PION          "♟"
#define CAVALIER      "🩒"

#define COLOR_RED     "\x1b[31m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_RESET   "\x1b[0m"   // Réinitialise la styles

typedef struct{
    char piece[TAILLE][TAILLE];
} Plateau;

typedef struct{
    Plateau plateau;
    int tour;
    int penalite[2]; // [0] => Joueur 1 && [1] => Joueur 2
} Jeu;

typedef struct {
    int ligne;
    int colonne;
} Position;

typedef struct{
    Position depart;
    Position arrivee;
    Position redeploiement; // {-1, -1} si invalide
} Deplacement;


int main(void){
    printf(VIDE "\n");
    printf(COLOR_BLUE PION COLOR_RESET "\n");
    printf(COLOR_RED CAVALIER COLOR_RESET "\n");
    return 0;
}
