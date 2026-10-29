// gcc ultime_pion.c -o ultime_pion && ./ultime_pion -Wall -std=c17
// TODO: delete command
#include <stdio.h>
#include <string.h> // Pour memcpy()
#include <stddef.h> // Pour size_t

#define TAILLE        5
#define BORNE_MIN     0
#define BORNE_MAX     (TAILLE - 1)

// Codes des cases : CA = cavalier, PI = pion, VI = case vide ; 1 et 2 désignent les joueurs.
#define CA_1          'C'
#define CA_2          'c'
#define PI_1          'P'
#define PI_2          'p'
#define VI            '.'

#define VIDE          "·"
#define PION          "♟"
#define CAVALIER      "🩒"

#define NB_DESTINATIONS_PION_MAX 3
#define NB_DESTINATIONS_CAVALIER_MAX 8 

#define COLOR_BLUE    "\x1b[34m"    // Joueur 1
#define COLOR_RED     "\x1b[31m"    // Joueur 2
#define COLOR_RESET   "\x1b[0m"     // Réinitialise les styles


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


int plateau_deplacement_valide(Plateau *plateau, Deplacement depl, int joueur);
int jeu_resoudre_tour(Jeu *jeu, Deplacement depl1, Deplacement depl2);
int jeu_est_termine(Jeu *jeu);
int jeu_vainqueur(Jeu *jeu);
void jeu_charger(Jeu *jeu);
void jeu_ecrire(Jeu *jeu);
int verifier_defaite_penalites(int penalite[]);
int aucun_pions_restant(char piece[][TAILLE]);
int deplacement_hors_plateau(Deplacement d);
int deplacement_cavalier_valide(Deplacement d);
int deplacement_pion_valide(Deplacement d, int joueur);
int est_joueur(int joueur);
int identifier_piece(Plateau *plateau, Deplacement d, int est_depart);
int est_piece_joueur(int joueur, int code_piece);
int pion_bloque(int code_piece_depart, int code_piece_arrivee, Deplacement d, int joueur);
int determiner_sens(int joueur);
int generer_deplacements_cavalier(Position depart, Deplacement d[], size_t capacite);
int generer_deplacements_pion(Position depart, Deplacement d[], size_t capacite, int joueur);
int aucun_coup_possible(Plateau *plateau);
void jeu_afficher(Jeu *jeu);
void jeu_initialiser(Jeu *jeu);
void plateau_promouvoir(Plateau *plateau, Position pos, int joueur);
int position_lire(Position *pos);
int derniere_ligne(Plateau *plateau, Deplacement d);
void deplacement_lire(Deplacement *d);
int peut_redeployer(Plateau *plateau, int joueur);
int redeploiement_valide(Plateau *plateau, Position pos, int joueur);
void redeploiement_lire(Deplacement *d, Plateau *plateau, int joueur);
int resoudre_deplacements(Jeu *jeu, Deplacement depl1, Deplacement depl2, int j1_valide, int j2_valide);
char resoudre_duel(char piece1, char piece2);
int resoudre_redeploiement(Jeu *jeu, Deplacement depl1, Deplacement depl2, int j1_valide, int j2_valide);
int main(void);

/**
 * @brief Vérifie si un déplacement est valide sur le plateau pour un joueur donné.
 *
 * @param plateau Pointeur vers le plateau de jeu.
 * @param depl Structure Deplacement contenant les positions de départ et d'arrivée.
 * @param joueur Numéro du joueur (1 ou 2) effectuant le déplacement.
 * @return 1 si le déplacement est valide (pièce du joueur à la position de départ,
 *         déplacement géométriquement correct, case d'arrivée non occupée par une pièce alliée,
 *         et règles spécifiques du pion respectées),
 *         0 sinon (joueur invalide, déplacement hors plateau, pièce inexistante ou alliée à l'arrivée,
 *         déplacement géométriquement invalide, ou pion bloqué).
 *
 * @note Ne gère pas les règles de redéploiement.
 */
int plateau_deplacement_valide(Plateau *plateau, Deplacement depl, int joueur){
    if (!est_joueur(joueur)){
        return 0;
    }

    if (deplacement_hors_plateau(depl)){
        return 0;
    }

    int piece_depart = identifier_piece(plateau, depl, 1);

    if (!est_piece_joueur(joueur, piece_depart)){
        return 0;
    }

    if (piece_depart == -1 || piece_depart == 1){
        if (!deplacement_cavalier_valide(depl)){
            return 0;
        }
    } else if (piece_depart == -2 || piece_depart == 2){
        if (!deplacement_pion_valide(depl, joueur)){
            return 0;
        }
    }

    int piece_arrivee = identifier_piece(plateau, depl, 0);

    if (pion_bloque(piece_depart, piece_arrivee, depl, joueur)){
        return 0;
    }

    int sens = determiner_sens(joueur);
    // Cas ou la piece c'est une piece du joueur
    if (piece_arrivee == 1 * sens || piece_arrivee == 2 * sens) // piece negative si c'est joueur 1, piece positive si c'est joueur 2
    {
        return 0;
    }

    return 1;
}

/**
 * @brief Résout un tour complet du jeu en traitant les déplacements et les promotions/redéploiements.
 *
 * Cette fonction gère la résolution d'un tour en deux phases :
 * 1. Résolution des déplacements simultanés des deux joueurs (duels, captures, pénalités).
 * 2. Traitement des promotions et redéploiements pour les pions atteignant la dernière ligne.
 *
 * @param jeu Pointeur vers la structure Jeu contenant l'état actuel du jeu.
 * @param depl1 Déplacement choisi par le joueur 1.
 * @param depl2 Déplacement choisi par le joueur 2.
 * @return 1 si le tour a été résolu avec succès.
 *
 */
int jeu_resoudre_tour(Jeu *jeu, Deplacement depl1, Deplacement depl2){
    int j1_valide = plateau_deplacement_valide(&jeu->plateau, depl1, 1);
    int j2_valide = plateau_deplacement_valide(&jeu->plateau, depl2, 2);
    jeu->tour += 1;
    
    // Phase 1 : Deplacements
    if (!resoudre_deplacements(jeu, depl1, depl2, j1_valide, j2_valide)){
        return 1; // Double pénalité, on passe au tour suivant
    }

    // Phase 2 : Redeploiement & Promotion
    resoudre_redeploiement(jeu, depl1, depl2, j1_valide, j2_valide);
    return 1;
}

/**
 * @brief Vérifie si la partie est terminée.
 *
 * @param jeu Pointeur vers l'état actuel du jeu.
 * @return 1 si la partie est terminée (aucun pion restant pour l'un des joueurs,
 *         aucun coup valide possible pour l'un des joueurs, ou un joueur a atteint 2 points de pénalité),
 *         0 sinon.
 */
int jeu_est_termine(Jeu *jeu){
    if (aucun_pions_restant(jeu->plateau.piece)){
        return 1;
    }

    if (aucun_coup_possible(&jeu->plateau)){
        return 1;
    }

    if (verifier_defaite_penalites(jeu->penalite)){
        return 1;
    }
    return 0;
}


/**
 * @brief Détermine le vainqueur d'une partie terminée.
 *
 * @param jeu Pointeur vers l'état actuel du jeu (supposé terminé).
 * @return Numéro du joueur vainqueur (1 ou 2),
 *         ou 0 en cas de match nul.
 *
 * @note Cette fonction suppose que la partie est terminée (appel préalable à jeu_est_termine()).
 *       Les règles de fin de partie sont vérifiées dans l'ordre suivant :
 *       1. Pénalités (un joueur atteint 2 points de pénalité),
 *       2. Pions restants (un joueur n'a plus de pions),
 *       3. Aucun coup valide possible (match nul).
 */
int jeu_vainqueur(Jeu *jeu){
    int cas3 = verifier_defaite_penalites(jeu->penalite);
    if (cas3 == -1){
        return 0;
    } else if (cas3 == 1 || cas3 == 2){
        return cas3;
    }

    int cas1 = aucun_pions_restant(jeu->plateau.piece);
    if (cas1 == -1){
        return 0;
    } else if (cas1 == 1 || cas1 == 2){
        return cas1; // joueur 1 ou joueur 2
    } 

    if (aucun_coup_possible(&jeu->plateau)){
        return 0;
    }

    return 0;
}

/**
 * @brief Charge l'état du jeu depuis l'entrée standard.
 *
 * @param jeu Pointeur vers la structure Jeu à remplir.
 *
 * Lit dans l'ordre :
 * - Le numéro du tour actuel,
 * - Les points de pénalité du joueur 1,
 * - Les points de pénalité du joueur 2,
 * - Les 25 caractères représentant l'état du plateau (TAILLE × TAILLE),
 *   où chaque caractère est lu en ignorant les espaces et retours à la ligne.
 *
 */
void jeu_charger(Jeu *jeu){
    scanf("%d", &jeu->tour);
    scanf("%d", &jeu->penalite[0]);
    scanf("%d", &jeu->penalite[1]);
    for (int i = 0; i < TAILLE; i++){
        for (int j = 0; j < TAILLE; j++){
            scanf(" %c", &jeu->plateau.piece[i][j]);
        }
    }
}

/**
 * @brief Écrit l'état actuel du jeu sur la sortie standard.
 *
 * @param jeu Pointeur vers la structure Jeu à afficher.
 *
 * Affiche dans l'ordre :
 * - Le numéro du tour actuel,
 * - Les points de pénalité du joueur 1 et du joueur 2 (séparés par un espace),
 * - Les 25 caractères représentant l'état du plateau (TAILLE × TAILLE),
 *   où chaque caractère est suivi d'un espace, et chaque ligne est terminée par un retour à la ligne.
 *
 */
void jeu_ecrire(Jeu *jeu){
    printf("%d\n", jeu->tour);
    printf("%d %d\n", jeu->penalite[0], jeu->penalite[1]);
    for (int i = 0; i < TAILLE; i++){
        for (int j = 0; j < TAILLE; j++){
            printf("%c ", jeu->plateau.piece[i][j]);
        }
        printf("\n");
    }
}

/**
 * @brief Vérifie si un ou les deux joueurs ont atteint 2 points de pénalité.
 *
 * @param penalite Tableau 1D représentant les points de pénalité des joueurs.
 * @return 1 si le joueur 2 a 2 points de pénalité,
 *         2 si le joueur 1 a 2 points de pénalité,
 *        -1 si les deux joueurs ont 2 points de pénalité,
 *         0 si aucun joueur n'a atteint 2 points de pénalité.
 */
int verifier_defaite_penalites(int penalite[]){
    if (penalite[0] == 2){
        if (penalite[1] == 2){
            return -1;
        }
        return 2;
    } else if (penalite[1] == 2){
        return 1;
    }
    return 0;
}

/**
 * @brief Détermine si un joueur n'a plus de pions, indiquant un vainqueur ou un match nul.
 *
 * @param piece Tableau 2D représentant les pièces du plateau.
 * @return 2 si le joueur 1 n'a plus de pions (victoire du joueur 2),
 *         1 si le joueur 2 n'a plus de pions (victoire du joueur 1),
 *        -1 si les deux joueurs n'ont plus de pions (match nul),
 *         0 si les deux joueurs ont encore des pions (partie non terminée).
 */
int aucun_pions_restant(char piece[][TAILLE]){
    // On suppose qu'ils n'ont aucun pions restants
    int aucun_pion_j1 = 1;
    int aucun_pion_j2 = 1;
    for (int i = 0; i < TAILLE; i++)
    {
        for (int j = 0; j < TAILLE; j++){
            if (piece[i][j] == PI_1 && aucun_pion_j1){
                aucun_pion_j1 = 0;
            } else if (piece[i][j] == PI_2 && aucun_pion_j2) {
                aucun_pion_j2 = 0;
            }

            if (!aucun_pion_j1 && !aucun_pion_j2){
                return 0; // Les deux joueurs ont des pions en jeu
            }
        }
    }
    
    if (aucun_pion_j1){
        if (aucun_pion_j2){
            return -1;
        }
        return 2;
    } else if (aucun_pion_j2){
        return 1;
    }

    return 0; // Cas déjà couvert mais nécessaire pour le compilateur
}

/**
 * @brief Vérifie si un déplacement sort des limites du plateau.
 *
 * @param d Structure Deplacement contenant les positions de départ et d'arrivée.
 * @return 1 si le départ ou l'arrivée est en dehors du plateau,
 *         0 sinon.
 */
int deplacement_hors_plateau(Deplacement d){
    return (
        d.depart.ligne < BORNE_MIN ||
        d.depart.colonne < BORNE_MIN ||
        d.arrivee.ligne < BORNE_MIN ||
        d.arrivee.colonne < BORNE_MIN ||
        d.depart.ligne > BORNE_MAX ||
        d.depart.colonne > BORNE_MAX ||
        d.arrivee.ligne > BORNE_MAX ||
        d.arrivee.colonne > BORNE_MAX);
}

/**
 * @brief Vérifie si un déplacement a la forme d'un saut de cavalier valide.
 *
 * @param d Structure Deplacement contenant les positions de départ et d'arrivée.
 * @return 1 si le déplacement est un saut de cavalier valide (en "L"),
 *         0 si le déplacement est invalide ou en dehors des limites du plateau.
 */
int deplacement_cavalier_valide(Deplacement d){
    // Cas ligne +- 2 et colonne +- 1
    if (d.arrivee.ligne == d.depart.ligne + 2 || d.arrivee.ligne == d.depart.ligne - 2)
    {
        if (d.arrivee.colonne == d.depart.colonne + 1 || d.arrivee.colonne == d.depart.colonne - 1){
            return 1;
        }
    } 
    
    // Cas ligne +- 1 et colonne +- 2
    if (d.arrivee.ligne == d.depart.ligne + 1 || d.arrivee.ligne == d.depart.ligne - 1){
        if (d.arrivee.colonne == d.depart.colonne + 2 || d.arrivee.colonne == d.depart.colonne - 2){
            return 1;
        }
    }

    return 0;
}


/**
 * @brief Vérifie si un déplacement a la forme valide pour un pion du joueur donné.
 *
 * @param d Structure Deplacement contenant les positions de départ et d'arrivée.
 * @param joueur Numéro du joueur (1 ou 2) dont le pion se déplace.
 * @return 1 si le déplacement est géométriquement valide pour un pion (avancer d'une case
 *         ou capturer en diagonale dans le bon sens),
 *         0 si le déplacement est invalide, en dehors des limites du plateau,
 *         ou si le joueur n'est pas 1 ou 2.
 *
 * @note Cette fonction ne vérifie pas si la case d'arrivée est libre ou contient un adversaire.
 */
int deplacement_pion_valide(Deplacement d, int joueur){
    // Cas ligne + 1 (joueur 1)
    if (d.arrivee.ligne != d.depart.ligne + 1 && joueur == 1){
        return 0; // joueur 1 doit forcément faire ligne + 1
    }

    // Cas ligne - 1 (joueur 2)
    if (d.arrivee.ligne != d.depart.ligne - 1 && joueur == 2){
        return 0; // joueur 2 doit forcément faire ligne - 1
    }

    // Avance tout droit ou mange en diagonale
    if (
        d.arrivee.colonne == d.depart.colonne - 1 ||
        d.arrivee.colonne == d.depart.colonne + 1 ||
        d.arrivee.colonne == d.depart.colonne
    ){
        return 1;
    }

    return 0;
}

/**
 * @brief Vérifie si le numéro de joueur est valide.
 *
 * @param joueur Numéro du joueur à vérifier (doit être 1 ou 2).
 * @return 1 si le joueur est valide (1 ou 2),
 *         0 sinon.
 */
int est_joueur(int joueur){
    if (joueur != 1 && joueur != 2){
        return 0;
    }
    return 1;
}

/**
 * @brief Identifie le contenu d'une case du plateau (départ ou arrivée).
 *
 * @param plateau Pointeur vers le plateau de jeu.
 * @param d Structure Deplacement contenant les positions de départ et d'arrivée.
 * @param est_depart 1 pour vérifier la position de départ, 0 pour l'arrivée.
 * @return -2 si la case contient PI_1,
 *         -1 si la case contient CA_1,
 *          0 si la case est vide VI,
 *          1 si la case contient CA_2,
 *          2 si la case contient PI_2,
 *         et 0 si est_depart n'est pas 0 ou 1.
 */
int identifier_piece(Plateau *plateau, Deplacement d, int est_depart){
    char piece;
    if (est_depart == 1){
        piece = plateau->piece[d.depart.ligne][d.depart.colonne];
    } else if (est_depart == 0){
        piece = plateau->piece[d.arrivee.ligne][d.arrivee.colonne];
    } else{
        return 0;
    }

    if (piece == CA_1) return -1;
    if (piece == PI_1) return -2;
    if (piece == CA_2) return 1;
    if (piece == PI_2) return 2;

    // Case vide
    return 0;
}

/**
 * @brief Vérifie si un code de pièce appartient au joueur donné.
 *
 * @param joueur Numéro du joueur (1 ou 2).
 * @param code_piece Code de la pièce retourné par identifier_piece().
 * @return 1 si la pièce appartient au joueur,
 *         0 si la pièce appartient à l'adversaire, si la case est vide (code_piece = 0),
 *         ou si le joueur n'est pas 1 ou 2.
 */
int est_piece_joueur(int joueur, int code_piece){
    if (joueur == 1){
        if (code_piece == -1 || code_piece == -2){
            return 1;
        }
    }

    if (joueur == 2){
        if (code_piece == 1 || code_piece == 2){
            return 2;
        }
    }

    return 0;
}

/**
 * @brief Vérifie si un pion est bloqué dans son déplacement.
 *
 * @param code_piece_depart Code de la pièce de départ (doit être -2 ou 2 pour un pion).
 * @param code_piece_arrivee Code de la pièce à l'arrivée (retourné par identifier_piece()).
 * @param d Structure Deplacement contenant les positions de départ et d'arrivée.
 * @param joueur Numéro du joueur (1 ou 2) dont le pion se déplace.
 * @return 1 si le pion est bloqué (tentative de déplacement diagonal vers une case vide,
 *         ou présence d'une pièce devant lui en ligne droite),
 *         0 sinon (déplacement valide ou pièce non bloquée).
 */
int pion_bloque(int code_piece_depart, int code_piece_arrivee, Deplacement d, int joueur){
    if (code_piece_depart != -2 && code_piece_depart != 2){
        return 0; // Pas un pion
    }

    int sens = determiner_sens(joueur);

    // Cas 1 : pion_depart ne peut pas manger une case vide (avancer en diagonale)
    if (code_piece_arrivee == 0){
        if (d.arrivee.ligne ==  d.depart.ligne - sens){
            if (d.arrivee.colonne == d.depart.colonne - 1 || d.arrivee.colonne == d.depart.colonne + 1){
                return 1;
            }
        }
        return 0; // le pion avance tt droit vers une case vide == pas bloque
    }

    // Cas 2 : une piece est devant le pion_depart
    if (d.arrivee.ligne == d.depart.ligne - sens && d.arrivee.colonne == d.depart.colonne){
        return 1;
    }

    return 0;
}

/**
 * @brief Détermine le sens de déplacement d'un joueur.
 *
 * @param joueur Numéro du joueur (1 ou 2).
 * @return -1 si le joueur est 1 (déplacement vers les lignes décroissantes),
 *          1 si le joueur est 2 (déplacement vers les lignes croissantes),
 *          0 si le joueur n'est pas valide (ni 1 ni 2).
 */
int determiner_sens(int joueur){
    if (joueur != 1 && joueur != 2) return 0;
    return (joueur == 1) ? -1 : 1;
}

/**
 * @brief Génère les déplacements candidats d'un cavalier à partir d'une position.
 *
 * @param depart Position de départ du cavalier.
 * @param d Tableau de structures Deplacement où stocker les déplacements candidats.
 * @param capacite Nombre d'éléments que peut contenir le tableau `d`.
 * @return 1 si les déplacements ont été générés avec succès (capacité suffisante),
 *         0 si la capacité du tableau `d` est insuffisante.
 *
 * @note Le tableau `d` doit pouvoir contenir au moins `NB_DESTINATIONS_CAVALIER_MAX` éléments.
 *       Les déplacements générés incluent toutes les cases accessibles par un cavalier,
 *       indépendamment de leur validité sur le plateau (vérification à faire séparément).
 */
int generer_deplacements_cavalier(Position depart, Deplacement d[], size_t capacite){
    Deplacement deplacements_candidats[NB_DESTINATIONS_CAVALIER_MAX] = {
        {depart, {depart.ligne + 2, depart.colonne - 1}, {-1, -1}}, // l+2 c-1
        {depart, {depart.ligne + 2, depart.colonne + 1}, {-1, -1}}, // l+2 c+1
        {depart, {depart.ligne - 2, depart.colonne - 1}, {-1, -1}}, // l-2 c-1
        {depart, {depart.ligne - 2, depart.colonne + 1}, {-1, -1}}, // l-2 c+1
        {depart, {depart.ligne + 1, depart.colonne - 2}, {-1, -1}}, // l+1 c-2
        {depart, {depart.ligne + 1, depart.colonne + 2}, {-1, -1}}, // l+1 c+2
        {depart, {depart.ligne - 1, depart.colonne - 2}, {-1, -1}}, // l-1 c-2
        {depart, {depart.ligne - 1, depart.colonne + 2}, {-1, -1}}, // l-1 c+2
    };

    if (capacite < NB_DESTINATIONS_CAVALIER_MAX){
        return 0;
    }

    memcpy(d, deplacements_candidats, sizeof deplacements_candidats);

    return 1;
}

/**
 * @brief Génère les déplacements candidats d'un pion à partir d'une position.
 *
 * @param depart Position de départ du pion.
 * @param d Tableau de structures Deplacement où stocker les déplacements candidats.
 * @param capacite Nombre d'éléments que peut contenir le tableau `d`.
 * @param joueur Numéro du joueur (1 ou 2) dont le pion se déplace.
 * @return 1 si les déplacements ont été générés avec succès (joueur valide et capacité suffisante),
 *         0 si le joueur est invalide ou si la capacité du tableau `d` est insuffisante.
 *
 * @note Le tableau `d` doit pouvoir contenir au moins `NB_DESTINATIONS_PION_MAX` éléments.
 *       Les déplacements générés incluent :
 *       - Une case en avant (ligne ±1 selon le joueur),
 *       - Deux cases en diagonale avant (ligne ±1, colonne ±1 selon le joueur).
 *       La validité de ces déplacements sur le plateau doit être vérifiée séparément.
 */
int generer_deplacements_pion(Position depart, Deplacement d[], size_t capacite, int joueur){
    if (!est_joueur(joueur)){
        return 0;
    }

    int sens = determiner_sens(joueur);
    Deplacement deplacements_candidats[NB_DESTINATIONS_PION_MAX] = {
        {depart, {depart.ligne - sens, depart.colonne}, {-1, -1}}, // l+-1
        {depart, {depart.ligne - sens, depart.colonne - 1}, {-1, -1}}, // l+-1 c-1
        {depart, {depart.ligne - sens, depart.colonne + 1}, {-1, -1}}, // l+-1 c+1
    };

    if (capacite < NB_DESTINATIONS_PION_MAX){
        return 0;
    }

    memcpy(d, deplacements_candidats, sizeof deplacements_candidats);

    return 1;
}

/**
 * @brief Vérifie si l'un des joueurs n'a aucun coup valide possible.
 *
 * @param plateau Pointeur vers le plateau de jeu.
 * @return 1 si au moins un joueur n'a aucun coup valide possible,
 *         0 si les deux joueurs ont au moins un coup valide.
 */
int aucun_coup_possible(Plateau *plateau){
    int aucun_coup[2] = {1, 1}; // Joueur1, Joueur2

    for (int i = 0; i < TAILLE; i++){
        for (int j = 0; j < TAILLE; j++){
            Position pos_depart = {i, j};
            int code_piece_depart = identifier_piece(plateau, (Deplacement){pos_depart, {0, 0}, {-1, -1}}, 1); // position arrivee et redeploiement ne servent à rien ici
            int joueur;

            if (code_piece_depart == -1 || code_piece_depart == -2){
                joueur = 1;
            } else if (code_piece_depart == 1 || code_piece_depart == 2) {
                joueur = 2;
            } else{
                continue;
            }

            // Le joueur a un coup possible, on skip
            if (!aucun_coup[joueur - 1]){
                continue;
            }

            Deplacement deplacements_candidats[NB_DESTINATIONS_CAVALIER_MAX]; // Taille suffisante pour pion et cavalier
            int nb_destinations_max;
            if (code_piece_depart == -1 || code_piece_depart == 1){
                nb_destinations_max = NB_DESTINATIONS_CAVALIER_MAX;
                generer_deplacements_cavalier(pos_depart, deplacements_candidats, NB_DESTINATIONS_CAVALIER_MAX);
            } else if (code_piece_depart == -2 || code_piece_depart == 2){
                nb_destinations_max = NB_DESTINATIONS_PION_MAX;
                generer_deplacements_pion(pos_depart, deplacements_candidats, NB_DESTINATIONS_PION_MAX, joueur);
            } else{
                continue;
            }

            for (int i = 0; i < nb_destinations_max; i++){
                int coup_valide = plateau_deplacement_valide(plateau, deplacements_candidats[i], joueur);
                if (coup_valide){
                    aucun_coup[joueur - 1] = 0;
                    break;
                }
            }

            if (!aucun_coup[0] && !aucun_coup[1]){
                return 0; // Partie toujours jouable
            }
        }
    }
    return 1;
}

/**
 * @brief Affiche l'état actuel du jeu dans la console.
 *
 * Cette fonction affiche :
 * - Le numéro du tour en cours
 * - Les pénalités des deux joueurs
 * - Le plateau de jeu avec les pièces positionnées
 *
 * Les pièces sont représentées avec les conventions suivantes :
 * - VI : Case vide (affichée avec le style VIDE)
 * - CA_1 : Cavalier du Joueur 1 (bleu)
 * - PI_1 : Pion du Joueur 1 (bleu)
 * - CA_2 : Cavalier du Joueur 2 (rouge)
 * - PI_2 : Pion du Joueur 2 (rouge)
 *
 * Le plateau est affiché avec :
 * - Des coordonnées en lettres (A-E) pour les colonnes
 * - Des coordonnées en chiffres (1-5) pour les lignes
 * - Des couleurs pour distinguer les pièces des joueurs
 *
 * @param jeu Pointeur vers la structure Jeu contenant l'état actuel du jeu
 */
void jeu_afficher(Jeu *jeu)
{
    printf("Tour: %d\n", jeu->tour);
    printf("[Pénalité Joueur 1] %d - %d [Pénalité Joueur 2]\n\n", jeu->penalite[0], jeu->penalite[1]);

    printf("    A  B  C  D  E\n");
    printf("  +---------------\n");

    for (int i = 0; i < TAILLE; i++) {
        printf("%d |", i + 1);

        for (int j = 0; j < TAILLE; j++) {
            switch (jeu->plateau.piece[i][j]) {
                case VI:
                    printf(" " VIDE " ");
                    break;
                case CA_1:
                    printf(" " COLOR_BLUE CAVALIER COLOR_RESET " ");
                    break;
                case PI_1:
                    printf(" " COLOR_BLUE PION COLOR_RESET " ");
                    break;
                case CA_2:
                    printf(" " COLOR_RED CAVALIER COLOR_RESET " ");
                    break;
                case PI_2:
                    printf(" " COLOR_RED PION COLOR_RESET " ");
                    break;
            }
        }
        printf("\n");
    }
    printf("\n");
}

/**
 * @brief Initialise un état de jeu avec la configuration de départ standard.
 *
 * @param jeu Pointeur vers la structure Jeu à initialiser.
 *
 * Place les pièces sur le plateau selon la configuration initiale :
 * - Joueur 1 (haut) : Cavaliers en A1/E1, pions en B1/C1/D1 et A2/E2.
 * - Joueur 2 (bas) : Cavaliers en A5/E5, pions en B5/C5/D5 et A4/E4.
 * Remet le numéro de tour à 1 et les pénalités des deux joueurs à 0.
 */
void jeu_initialiser(Jeu *jeu){
    const char pieces_initiales[TAILLE][TAILLE] = {
        {CA_1, PI_1, PI_1, PI_1, CA_1},
        {PI_1, VI, VI, VI, PI_1},
        {VI, VI, VI, VI, VI},
        {PI_2, VI, VI, VI, PI_2},
        {CA_2, PI_2, PI_2, PI_2, CA_2}
    };

    for (int i = 0; i < TAILLE; i++) {
        for (int j = 0; j < TAILLE; j++) {
            jeu->plateau.piece[i][j] = pieces_initiales[i][j];
        }
    }

    jeu->tour = 1;
    jeu->penalite[0] = 0;
    jeu->penalite[1] = 0;
}

/**
 * @brief Promouvoit un pion en cavalier à la position donnée.
 *
 * Remplace directement la pièce à la position `pos` par un cavalier du joueur spécifié.
 *
 * @param plateau Pointeur vers le plateau de jeu
 * @param pos Position de la pièce à promouvoir
 * @param joueur Numéro du joueur (1 ou 2)
 */
void plateau_promouvoir(Plateau *plateau, Position pos, int joueur){
    plateau->piece[pos.ligne][pos.colonne] = (joueur == 1) ? CA_1 : CA_2;
}

/**
 * @brief Lit une position au format "ligne colonne" depuis l'entrée standard.
 *
 * Cette fonction lit deux caractères depuis l'entrée standard (stdin) :
 * - Le premier caractère représente la ligne (chiffre de '1' à '5').
 * - Le second caractère représente la colonne (lettre majuscule de 'A' à 'E').
 *
 * La position lue est convertie en indices de tableau (0-based) :
 * - La ligne est décrémentée de 1 (ex: '1' devient 0).
 * - La colonne est convertie en indice numérique (ex: 'A' devient 0).
 *
 * @param pos Pointeur vers la structure Position à remplir avec les valeurs lues.
 * @return int 0 si la lecture a réussi, 1 en cas d'erreur (format invalide ou EOF).
 */
int position_lire(Position *pos){
    char colonne, ligne;

    if (scanf(" %c %c", &colonne, &ligne) != 2) {
        return 1;
    }

    pos->ligne = ligne - '0' - 1; // Conversion d'un entier char en entier int - 1 (indice tableau)
    pos->colonne = colonne - 'A';

    return 0;
}

/**
 * @brief Vérifie si un déplacement atteint la dernière ligne du plateau pour une pièce spécifique.
 *
 * Cette fonction détermine si un déplacement permet à une pièce d'atteindre la dernière ligne du plateau,
 * ce qui est utilisé pour déclencher une promotion.
 *
 * Conditions vérifiées :
 * - La pièce au départ doit être un pion.
 * - Si c'est un pion bleu (-2), il doit atteindre la ligne maximale (BORNE_MAX).
 * - Si c'est un pion rouge (2), il doit atteindre la ligne minimale (BORNE_MIN).
 *
 * @param plateau Pointeur vers la structure Plateau représentant l'état actuel du jeu.
 * @param d Structure Déplacement contenant les positions de départ et d'arrivée.
 * @return int 1 si le déplacement atteint la dernière ligne pour la pièce concernée, 0 sinon.
 */
int derniere_ligne(Plateau *plateau, Deplacement d) {
    int piece = identifier_piece(plateau, d, 1);

    if (piece == -2) {
        return d.depart.ligne == BORNE_MAX - 1
            && d.arrivee.ligne == BORNE_MAX;
    }

    if (piece == 2) {
        return d.depart.ligne == BORNE_MIN + 1
            && d.arrivee.ligne == BORNE_MIN;
    }

    return 0;
}

/**
 * @brief Lit un déplacement depuis l'entrée standard (stdin) au format "ligne colonne".
 *
 * Cette fonction demande à l'utilisateur de saisir deux positions :
 * - Une position de départ.
 * - Une position d'arrivée.
 * 
 * En cas d'erreur de saisie, l'utilisateur est invité à recommencer.
 *
 * @param d Pointeur vers une structure Deplacement qui sera remplie avec les positions
 * de départ et d'arrivée saisies par l'utilisateur.
 */
void deplacement_lire(Deplacement *d){
    Position depart, arrivee;
    int pos;
    do{
        printf("Saisir position départ : ");
        pos = position_lire(&depart);

        if (pos == 1){
            printf("\nUne erreur a survenue.\n");
            continue;
        }
        
        printf("Saisir position arrivée : ");
        pos = position_lire(&arrivee);

        if (pos == 1){
            printf("\nUne erreur a survenue.\n");
            continue;
        }
    } while (pos == 1);

    d->depart = depart;
    d->arrivee = arrivee;
    d->redeploiement = (Position){-1, -1}; // Par défaut on va le laisser invalides
}

/**
 * @brief Vérifie si un joueur peut effectuer un redéploiement de pion.
 *
 * Un joueur peut redéployer un pion s'il possède déjà deux cavaliers sur le plateau.
 * Cette fonction compte le nombre de cavaliers du joueur et retourne 1 s'il en a au moins deux.
 *
 * @param plateau Pointeur vers le plateau de jeu.
 * @param joueur Numéro du joueur (1 ou 2).
 * @return 1 si le joueur peut redéployer (a déjà 2 cavaliers), 0 sinon ou si le joueur est invalide.
 */
int peut_redeployer(Plateau *plateau, int joueur){
    if (!est_joueur(joueur)){
        return 0;
    }

    char cavalier = (joueur == 1) ? CA_1 : CA_2;
    int nb_cavalier = 0;

    for (int i = 0; i < TAILLE; i++){
        for (int j = 0; j < TAILLE; j++){
            if (plateau->piece[i][j] == cavalier){
                nb_cavalier++;
            }

            if (nb_cavalier >= 2){
                return 1;
            }
        }
    }

    return 0;
}

/**
 * @brief Vérifie si un redéploiement de pion est valide.
 *
 * Un redéploiement est valide si :
 * - La position est dans les limites du plateau.
 * - Le joueur est valide (1 ou 2).
 * - La position n'est pas sur la dernière ligne du joueur (BORNE_MAX pour le joueur 1, BORNE_MIN pour le joueur 2).
 * - La case contient bien un vide (VI).
 *
 * @param plateau Pointeur vers le plateau de jeu.
 * @param pos Position à vérifier pour le redéploiement.
 * @param joueur Numéro du joueur (1 ou 2).
 * @return 1 si le redéploiement est valide, 0 sinon.
 */
int redeploiement_valide(Plateau *plateau, Position pos, int joueur){
    if (deplacement_hors_plateau((Deplacement){pos, pos})){
        return 0;
    }

    if (!est_joueur(joueur)){
        return 0;
    }

    if (joueur == 1 && pos.ligne == BORNE_MAX){
        return 0;
    }

    if (joueur == 2 && pos.ligne == BORNE_MIN){
        return 0;
    }

    if (plateau->piece[pos.ligne][pos.colonne] != VI){
        return 0;
    }

    return 1;
}

/**
 * @brief Lit une position de redéploiement valide pour un pion promu.
 *
 * Demande à l'utilisateur de saisir une position de redéploiement jusqu'à ce qu'elle soit valide.
 * Une position est valide si elle respecte les règles de redéploiement (case vide et non sur la dernière ligne).
 *
 * @param d Pointeur vers le déplacement à compléter avec la position de redéploiement.
 * @param plateau Pointeur vers le plateau de jeu.
 * @param joueur Numéro du joueur (1 ou 2) effectuant le redéploiement.
 */
void redeploiement_lire(Deplacement *d, Plateau *plateau, int joueur){
    Position redeploiement;
    int pos;
    do
    {
        printf("Saisir position redeploiement :");
        pos = position_lire(&redeploiement);

        if (pos == 1){
            printf("\nUne erreur a survenue.\n");
            continue;
        }

        
    } while (pos == 1 || !redeploiement_valide(plateau, redeploiement, joueur));

    d->redeploiement = redeploiement;
}

/**
 * @brief Résout les déplacements simultanés des deux joueurs.
 *
 * Gère les cas suivants :
 * - Déplacements invalides (pénalités appliquées).
 * - Duel sur la même case (résolution via resoudre_duel).
 * - Déplacements normaux ou captures sur cases distinctes.
 *
 * @param jeu Pointeur vers la structure Jeu contenant l'état du plateau et les pénalités.
 * @param depl1 Déplacement du joueur 1.
 * @param depl2 Déplacement du joueur 2.
 * @param j1_valide Indique si le déplacement du joueur 1 est valide (1) ou non (0).
 * @param j2_valide Indique si le déplacement du joueur 2 est valide (1) ou non (0).
 * @return 1 si la résolution s'est déroulée sans erreur, 0 en cas de double pénalité.
 */
int resoudre_deplacements(Jeu *jeu, Deplacement depl1, Deplacement depl2, int j1_valide, int j2_valide){
    // Cas 1 : Les deux joueurs ont un déplacement invalide
    if (!j1_valide && !j2_valide){
        jeu->penalite[0] += 1;
        jeu->penalite[1] += 1;
        return 0;
    }
    
    // Récupération des pieces de départ
    char j1_piece_depart = '\0', j2_piece_depart = '\0';
    
    if (j1_valide){
        j1_piece_depart = jeu->plateau.piece[depl1.depart.ligne][depl1.depart.colonne];
        jeu->plateau.piece[depl1.depart.ligne][depl1.depart.colonne] = VI; 
    } else {
        jeu->penalite[0] += 1;
    }
    
    if (j2_valide){
        j2_piece_depart = jeu->plateau.piece[depl2.depart.ligne][depl2.depart.colonne]; 
        jeu->plateau.piece[depl2.depart.ligne][depl2.depart.colonne] = VI; 
    } else {
        jeu->penalite[1] += 1;
    }

    // Cas 2 : Duel sur même case (arrivée)
    if (j1_valide && j2_valide){
        if (depl1.arrivee.ligne == depl2.arrivee.ligne && depl1.arrivee.colonne == depl2.arrivee.colonne){
            jeu->plateau.piece[depl1.arrivee.ligne][depl1.arrivee.colonne] = resoudre_duel(j1_piece_depart, j2_piece_depart);
            return 1;
        }
    }

    // Cas 3 : Capture sur case distinct 
    if (j1_valide){
        jeu->plateau.piece[depl1.arrivee.ligne][depl1.arrivee.colonne] = j1_piece_depart;
    }
    if (j2_valide){
        jeu->plateau.piece[depl2.arrivee.ligne][depl2.arrivee.colonne] = j2_piece_depart;
    }
    return 1;
}

/**
 * @brief Résout un duel entre deux pièces arrivant sur la même case.
 *
 * Applique les règles de résolution des duels :
 * - Deux pions s'éliminent mutuellement (retourne VI).
 * - Un cavalier l'emporte sur un pion (retourne le cavalier).
 * - Deux cavaliers s'éliminent mutuellement (retourne VI).
 *
 * @param piece1 Pièce du joueur 1 (PI_1 ou CA_1).
 * @param piece2 Pièce du joueur 2 (PI_2 ou CA_2).
 * @return Le caractère représentant la pièce gagnante, ou VI si les deux pièces sont éliminées.
 */
char resoudre_duel(char piece1, char piece2){
    switch(piece1){
        case PI_1:
            if (piece2 == PI_2){
                return VI;
            } else if (piece2 == CA_2){
                return CA_2;
            }
            break;
        case CA_1:
            if (piece2 == PI_2){
                return CA_1;
            } else if (piece2 == CA_2){
                return VI;
            }
    }
    return VI;
}

/**
 * @brief Résout les cas de promotion et redéploiement des pions atteignant la dernière ligne.
 *
 * Gère les scénarios suivants pour les déplacements valides uniquement :
 * - Aucun pion valide n'atteint la dernière ligne (retourne 0).
 * - Deux pions valides atteignent la dernière ligne et tentent un redéploiement sur la même case
 *   (les deux pions sont éliminés).
 * - Redéploiement valide sur des cases distinctes (le pion est déplacé si la case est vide).
 * - Promotion directe en cavalier si aucun redéploiement n'est spécifié.
 *
 * @param jeu Pointeur vers la structure Jeu contenant l'état du plateau.
 * @param depl1 Déplacement du joueur 1 (peut inclure une position de redéploiement).
 * @param depl2 Déplacement du joueur 2 (peut inclure une position de redéploiement).
 * @param j1_valide Indique si le déplacement du joueur 1 est valide (1) ou non (0).
 * @param j2_valide Indique si le déplacement du joueur 2 est valide (1) ou non (0).
 * @return 1 si un redéploiement ou une promotion a été traité, 0 sinon.
 */
int resoudre_redeploiement(Jeu *jeu, Deplacement depl1, Deplacement depl2, int j1_valide, int j2_valide){
    int j1_derniere_ligne = j1_valide 
    && depl1.arrivee.ligne == BORNE_MAX 
    && jeu->plateau.piece[depl1.arrivee.ligne][depl1.arrivee.colonne] == PI_1;
    
    int j2_derniere_ligne = j2_valide 
    && depl2.arrivee.ligne == BORNE_MIN 
    && jeu->plateau.piece[depl2.arrivee.ligne][depl2.arrivee.colonne] == PI_2;

    int j1_peut_redeployer = peut_redeployer(&jeu->plateau, 1);
    int j2_peut_redeployer = peut_redeployer(&jeu->plateau, 2);

    // Cas 1 : Personne dans la derniere ligne
    if (!j1_derniere_ligne && !j2_derniere_ligne){
        return 0;
    }

    // Cas 2 : Redeploiement dans la même case
    if (j1_derniere_ligne && j2_derniere_ligne){
        if (j1_peut_redeployer &&
            j2_peut_redeployer && 
            depl1.redeploiement.ligne == depl2.redeploiement.ligne &&
            depl1.redeploiement.colonne == depl2.redeploiement.colonne)
        {
            jeu->plateau.piece[depl1.arrivee.ligne][depl1.arrivee.colonne] = VI;
            jeu->plateau.piece[depl2.arrivee.ligne][depl2.arrivee.colonne] = VI;
            return 1;
        }
    }

    int redeploiement_possible;
    if (j1_derniere_ligne){
        // Cas 3 : Redeploiement dans des cases distinct
        if (j1_peut_redeployer){
            redeploiement_possible = redeploiement_valide(&jeu->plateau, depl1.redeploiement, 1);
            jeu->plateau.piece[depl1.arrivee.ligne][depl1.arrivee.colonne] = VI;
            if (redeploiement_possible){
                jeu->plateau.piece[depl1.redeploiement.ligne][depl1.redeploiement.colonne] = PI_1;
            }
        } else {
            // Cas 4 : Promotion
            plateau_promouvoir(&jeu->plateau, depl1.arrivee, 1);
        }
    }

    if (j2_derniere_ligne){
        if (j2_peut_redeployer){
            redeploiement_possible = redeploiement_valide(&jeu->plateau, depl2.redeploiement, 2);
            jeu->plateau.piece[depl2.arrivee.ligne][depl2.arrivee.colonne] = VI;
            if (redeploiement_possible){
                jeu->plateau.piece[depl2.redeploiement.ligne][depl2.redeploiement.colonne] = PI_2;
            }
        } else {
            plateau_promouvoir(&jeu->plateau, depl2.arrivee, 2);
        }
    }
    return 1;
}

int main(void){
    Jeu jeu;
    jeu_initialiser(&jeu);
    Deplacement depl1, depl2;
    while (!jeu_est_termine(&jeu)){
        jeu_afficher(&jeu);
        
        printf("--Joueur 1--\n");
        deplacement_lire(&depl1);
        if (!deplacement_hors_plateau(depl1) && derniere_ligne(&jeu.plateau, depl1)){ // Même si il ne peux pas redeployer, on anticipe le cas ou nb_cavalier < 2
            redeploiement_lire(&depl1, &jeu.plateau, 1);
        }
        
        printf("--Joueur 2--\n");
        deplacement_lire(&depl2);
        if (!deplacement_hors_plateau(depl2) && derniere_ligne(&jeu.plateau, depl2)){ 
            redeploiement_lire(&depl2, &jeu.plateau, 2);
        }

        jeu_resoudre_tour(&jeu, depl1, depl2);
    }

    int vainqueur = jeu_vainqueur(&jeu);
    switch(vainqueur){
        case 0:
            printf("\nPartie nulle.\n");
            break;
        case 1:
            printf("\nJoueur 1 a gagné.\n");
            break;
        case 2:
            printf("\nJoueur 2 a gagné.\n");
            break;
        }

    return 0;
}
