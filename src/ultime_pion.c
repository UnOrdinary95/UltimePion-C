// gcc ultime_pion.c -o ultime_pion && ./ultime_pion -Wall -std=c17
// TODO: delete command

#define TAILLE 5
#define VIDE "." 
#define J1_PION "P"
#define J1_CAVALIER "C"
#define J2_PION "p"
#define J2_CAVALIER "c"

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
    return 0;
}
