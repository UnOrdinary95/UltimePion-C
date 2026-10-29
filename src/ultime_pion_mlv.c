#include <MLV/MLV_all.h>
#include <stdio.h>


#define TAILLE          5
#define TAILLE_CARREE   100
#define COLOR_CHARCOAL  MLV_rgba(48, 46, 43, 255)
#define COLOR_GREEN     MLV_rgba(115, 149, 82, 255)
#define COLOR_CREAM     MLV_rgba(255, 255, 221, 255)
#define CA_1          'C'
#define CA_2          'c'
#define PI_1          'P'
#define PI_2          'p'
#define VI            '.'


int load_image(MLV_Image **image, char *path);
int load_pieces(MLV_Image **pionB, MLV_Image **pionW, MLV_Image **cavalierB, MLV_Image **cavalierW);
void draw_board(const char piece[][TAILLE], MLV_Image *pionB, 
    MLV_Image *pionW, MLV_Image *cavalierB, MLV_Image *cavalierW);
void refresh_board(const char piece[][TAILLE], MLV_Image *pionB, 
    MLV_Image *pionW, MLV_Image *cavalierB, MLV_Image *cavalierW);

int load_image(MLV_Image **image, char *path){
    *image = MLV_load_image(path);

    if (*image == NULL){
        printf("Impossible de charger l'image: %s\n", path);
        return 0;
    }

    return 1;
}

int load_pieces(MLV_Image **pionB, MLV_Image **pionW, MLV_Image **cavalierB, MLV_Image **cavalierW){
    if (!load_image(pionB, "assets/images/bP.png")){
        return 0;
    }
    if (!load_image(pionW, "assets/images/wP.png")){
        return 0;
    }
    if (!load_image(cavalierB, "assets/images/bN.png")){
        return 0;
    }
    if (!load_image(cavalierW, "assets/images/wN.png")){
        return 0;
    }
    return 1;
}

void draw_board(const char piece[][TAILLE], MLV_Image *pionB, MLV_Image *pionW, 
    MLV_Image *cavalierB, MLV_Image *cavalierW){
    int is_cream = 1;
    int x = 0, y = 0;
    for (int i = 0; i < TAILLE; i++, x = 0, y += TAILLE_CARREE){
        for (int j = 0; j < TAILLE; j++, x += TAILLE_CARREE){
            if (is_cream > 0){
                MLV_draw_filled_rectangle(x, y, TAILLE_CARREE, TAILLE_CARREE, COLOR_CREAM);
                is_cream *= -1;
            } else{
                MLV_draw_filled_rectangle(x, y, TAILLE_CARREE, TAILLE_CARREE, COLOR_GREEN);
                is_cream *= -1;
            }

            switch(piece[i][j]){
                case CA_1:
                    MLV_resize_image(cavalierB, TAILLE_CARREE, TAILLE_CARREE);
                    MLV_draw_image(cavalierB, x, y);
                    break;
                case PI_1:
                    MLV_resize_image(pionB, TAILLE_CARREE, TAILLE_CARREE);
                    MLV_draw_image(pionB, x, y);
                    break;
                case CA_2:
                    MLV_resize_image(cavalierW, TAILLE_CARREE, TAILLE_CARREE);
                    MLV_draw_image(cavalierW, x, y);
                    break;
                case PI_2:
                    MLV_resize_image(pionW, TAILLE_CARREE, TAILLE_CARREE);
                    MLV_draw_image(pionW, x, y);
                    break;
            }
        }
    }
}

void refresh_board(const char piece[][TAILLE], MLV_Image *pionB, MLV_Image *pionW, 
    MLV_Image *cavalierB, MLV_Image *cavalierW){
    MLV_clear_window(COLOR_CHARCOAL);
    draw_board(piece, pionB, pionW, cavalierB, cavalierW);
    MLV_update_window();
}

int main(int argc, char *argv[]){
        const char piece[TAILLE][TAILLE] = {
            {CA_1, PI_1, PI_1, PI_1, CA_1},
            {PI_1, VI, VI, VI, PI_1},
            {VI, VI, VI, VI, VI},
            {PI_2, VI, VI, VI, PI_2},
            {CA_2, PI_2, PI_2, PI_2, CA_2}
        };
        // Créé et affiche la fenêtre
        MLV_create_window("Ultime Pion MLV", "Ultime Pion MLV", 600, 600);
        MLV_clear_window(COLOR_CHARCOAL);

        MLV_Image *pionB = NULL;
        MLV_Image *pionW = NULL;
        MLV_Image *cavalierB = NULL;
        MLV_Image *cavalierW = NULL;

        if (!load_pieces(&pionB, &pionW, &cavalierB, &cavalierW)){
            if (pionB != NULL) MLV_free_image(pionB);
            if (pionW != NULL) MLV_free_image(pionW);
            if (cavalierB != NULL) MLV_free_image(cavalierB);
            if (cavalierW != NULL) MLV_free_image(cavalierW);
            MLV_free_window();
            return 1;
        }

        // MLV_resize_image(pionW, 100, 100);
        // MLV_draw_image(pionW, 0, 0);

        draw_board(piece, pionB, pionW, cavalierB, cavalierW);
        // Met à jour l'affichage.
        MLV_update_window();

        int x, y;
        MLV_wait_mouse(&x, &y);
 
        // Fermer la fenêtre
        MLV_free_window();
        MLV_free_image(pionB);
        MLV_free_image(pionW);
        MLV_free_image(cavalierB);
        MLV_free_image(cavalierW);
        return 0;
}
