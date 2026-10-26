// gcc ultime_pion.c -o ultime_pion && ./ultime_pion -Wall -std=c17
// TODO: delete command
#include <stdio.h>
#include <string.h> // Pour memcpy()
#include <stddef.h> // Pour size_t

#define TAILLE        5
#define BORNE_MIN     0
#define BORNE_MAX     TAILLE - 1

#define VIDE          "·"
#define PION          "♟"
#define CAVALIER      "🩒"

#define NB_DESTINATIONS_PION_MAX 3
#define NB_DESTINATIONS_CAVALIER_MAX 8 

#define COLOR_BLUE    "\x1b[34m"    // Joueur 1
#define COLOR_RED     "\x1b[31m"    // Joueur 2
#define COLOR_RESET   "\x1b[0m"     // Réinitialise la styles


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
int plateau_promouvoir(Plateau *plateau, Position pos);
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
            if (piece[i][j] == 'P' && aucun_pion_j1){
                aucun_pion_j1 = 0;
            } else if (piece[i][j] == 'p' && aucun_pion_j2) {
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
    // Position en dehors des bornes
    if (deplacement_hors_plateau(d)){
        return 0; // TODO: Delete si redondant?
    }
    
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
    // Le joueur doit etre valide
    if (!est_joueur(joueur)){ // TODO: Delete si redondant?
        return 0; 
    }

    if (deplacement_hors_plateau(d)){
        return 0; // TODO: Delete si redondant?
    }

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
 * @return -2 si la case contient un pion du joueur 1 ('P'),
 *         -1 si la case contient un cavalier du joueur 1 ('C'),
 *          0 si la case est vide ('.'),
 *          1 si la case contient un cavalier du joueur 2 ('c'),
 *          2 si la case contient un pion du joueur 2 ('p'),
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

    if (piece == 'C') return -1;
    if (piece == 'P') return -2;
    if (piece == 'c') return 1;
    if (piece == 'p') return 2;

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
 * - '.' : Case vide (affichée avec le style VIDE)
 * - 'C' : Cavalier du Joueur 1 (bleu)
 * - 'P' : Pion du Joueur 1 (bleu)
 * - 'c' : Cavalier du Joueur 2 (rouge)
 * - 'p' : Pion du Joueur 2 (rouge)
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
                case '.':
                    printf(" " VIDE " ");
                    break;
                case 'C':
                    printf(" " COLOR_BLUE CAVALIER COLOR_RESET " ");
                    break;
                case 'P':
                    printf(" " COLOR_BLUE PION COLOR_RESET " ");
                    break;
                case 'c':
                    printf(" " COLOR_RED CAVALIER COLOR_RESET " ");
                    break;
                case 'p':
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
        {'C', 'P', 'P', 'P', 'C'},
        {'P', '.', '.', '.', 'P'},
        {'.', '.', '.', '.', '.'},
        {'p', '.', '.', '.', 'p'},
        {'c', 'p', 'p', 'p', 'c'}
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
 * @brief Promouvoit un pion en cavalier s'il atteint le bord opposé du plateau.
 *
 * Cette fonction vérifie si un pion peut être promu en cavalier selon les règles suivantes :
 * - Le pion doit appartenir au joueur actuel
 * - Le pion doit avoir atteint la ligne opposée à sa ligne de départ (BORNE_MAX pour le Joueur 1, BORNE_MIN pour le Joueur 2)
 * - Le joueur ne doit pas déjà avoir 2 cavaliers sur le plateau
 *
 * @param plateau Pointeur vers la structure Plateau représentant l'état actuel du jeu
 * @param pos Position de la pièce à potentiellement promouvoir
 *
 * @return int
 * - 1 si la promotion a été effectuée avec succès
 * - 0 si la promotion n'est pas possible (position invalide, pièce non promouvable, limite de cavaliers atteinte, etc.)
 *
 */
int plateau_promouvoir(Plateau *plateau, Position pos){
    if (deplacement_hors_plateau((Deplacement){pos, pos, {-1, -1}})){
        return 0;
    }

    int code_piece = identifier_piece(plateau, (Deplacement){pos, {0, 0}, {-1, -1}}, 1);
    if (code_piece == 0 || code_piece == -1 || code_piece == 1){
        return 0;
    }

    int joueur;
    char cavalier;
    if (code_piece == -2){
        if (pos.ligne != BORNE_MAX){
            return 0;
        }
        joueur = 1;
        cavalier = 'C';
    } else {
        if (pos.ligne != BORNE_MIN){
            return 0;
        }
        joueur = 2;
        cavalier = 'c';
    }

    if (!est_piece_joueur(joueur, code_piece)){
        return 0;
    }

    int nb_cavalier = 0;
    for (int i = 0; i < TAILLE; i++){
        for (int j = 0; j < TAILLE; j++){
            if (plateau->piece[i][j] == cavalier){
                nb_cavalier++;
            }

            if (nb_cavalier >= 2){
                return 0;
            }
        }
    }
    plateau->piece[pos.ligne][pos.colonne] = cavalier;
    return 1;
}

int main(void){
    Jeu jeu;
    jeu_initialiser(&jeu);
    jeu_afficher(&jeu);
    return 0;
}
