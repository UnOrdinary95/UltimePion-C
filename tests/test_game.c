/* Keep the submitted source intact, replacing only its entry point. */
#define main ultime_pion_interactive_main
#include "../src/ultime_pion.c"
#undef main
#include <stdlib.h>

static unsigned long checks;
#define CHECK(condition) do { \
    ++checks; \
    if (!(condition)) { \
        fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #condition); \
        return 1; \
    } \
} while (0)

static Jeu empty_game(void) {
    Jeu game = {0};
    memset(game.plateau.piece, '.', sizeof game.plateau.piece);
    game.tour = 7;
    return game;
}

static Deplacement move(int r, int c, int rr, int cc) {
    return (Deplacement){{r, c}, {rr, cc}, {-1, -1}};
}

static int same_game(const Jeu *actual, const Jeu *expected) {
    if (actual->tour != expected->tour ||
        actual->penalite[0] != expected->penalite[0] ||
        actual->penalite[1] != expected->penalite[1] ||
        memcmp(actual->plateau.piece, expected->plateau.piece, 25)) {
        fprintf(stderr, "Expected turn %d, penalties %d %d; got %d, %d %d\n",
                expected->tour, expected->penalite[0], expected->penalite[1],
                actual->tour, actual->penalite[0], actual->penalite[1]);
        for (int r = 0; r < 5; ++r) {
            fprintf(stderr, "expected %.5s | actual %.5s\n",
                    expected->plateau.piece[r], actual->plateau.piece[r]);
        }
        return 0;
    }
    return 1;
}

static int initialization(void) {
    Jeu game;
    jeu_initialiser(&game);
    CHECK(game.tour == 1);
    CHECK(game.penalite[0] == 0 && game.penalite[1] == 0);
    CHECK(memcmp(game.plateau.piece, "CPPPC" "P...P" "....." "p...p" "cpppc", 25) == 0);
    CHECK(jeu_est_termine(&game) == 0);
    return 0;
}

/* Independent oracle: use only the rules, not the program's helper functions. */
static int expected_valid(char piece, char target, int r, int c, int rr, int cc, int player) {
    int own = player == 1 ? (piece == 'P' || piece == 'C') : (piece == 'p' || piece == 'c');
    int ally = player == 1 ? (target == 'P' || target == 'C') : (target == 'p' || target == 'c');
    if (!own || ally || (r == rr && c == cc)) return 0;
    int dr = rr - r, dc = abs(cc - c);
    if (piece == 'C' || piece == 'c')
        return (abs(dr) == 2 && dc == 1) || (abs(dr) == 1 && dc == 2);
    return dr == (player == 1 ? 1 : -1) &&
        ((dc == 0 && target == '.') || (dc == 1 && target != '.'));
}

static int validity(void) {
    const char pieces[] = ".PCpc";
    for (int player = 1; player <= 2; ++player)
    for (int p = 0; p < 5; ++p)
    for (int t = 0; t < 5; ++t)
    for (int r = 0; r < 5; ++r)
    for (int c = 0; c < 5; ++c)
    for (int rr = 0; rr < 5; ++rr)
    for (int cc = 0; cc < 5; ++cc) {
        if (r == rr && c == cc && p != t) continue;
        Jeu game = empty_game();
        game.plateau.piece[r][c] = pieces[p];
        game.plateau.piece[rr][cc] = pieces[t];
        Jeu before = game;
        int expected = expected_valid(pieces[p], pieces[t], r, c, rr, cc, player);
        int actual = plateau_deplacement_valide(&game.plateau, move(r, c, rr, cc), player);
        if (actual != expected) {
            fprintf(stderr, "player=%d piece=%c target=%c (%d,%d)->(%d,%d): expected %d got %d\n",
                    player, pieces[p], pieces[t], r, c, rr, cc, expected, actual);
        }
        CHECK(actual == expected);
        CHECK(same_game(&game, &before));
    }
    return 0;
}

static int boundaries(void) {
    Jeu game;
    jeu_initialiser(&game);
    CHECK(!plateau_deplacement_valide(&game.plateau, move(0, 0, 2, 1), 0));
    CHECK(!plateau_deplacement_valide(&game.plateau, move(0, 0, 2, 1), 3));
    for (int field = 0; field < 4; ++field) {
        for (int value = -1; value <= 5; value += 6) {
            Deplacement d = move(0, 0, 2, 1);
            if (field == 0) d.depart.ligne = value;
            if (field == 1) d.depart.colonne = value;
            if (field == 2) d.arrivee.ligne = value;
            if (field == 3) d.arrivee.colonne = value;
            CHECK(plateau_deplacement_valide(&game.plateau, d, 1) == 0);
        }
    }
    game = empty_game();
    for (int player = 1; player <= 2; ++player)
    for (int r = -1; r <= 5; ++r)
    for (int c = -1; c <= 5; ++c) {
        int expected = r >= 0 && r < 5 && c >= 0 && c < 5 && r != (player == 1 ? 4 : 0);
        CHECK(redeploiement_valide(&game.plateau, (Position){r, c}, player) == expected);
    }
    game.plateau.piece[2][2] = 'P';
    CHECK(!redeploiement_valide(&game.plateau, (Position){2, 2}, 1));
    return 0;
}

static int endings(void) {
    for (int pawns = 0; pawns < 4; ++pawns)
    for (int p1 = 0; p1 <= 2; ++p1)
    for (int p2 = 0; p2 <= 2; ++p2) {
        Jeu game = empty_game();
        game.plateau.piece[0][0] = 'C';
        game.plateau.piece[4][4] = 'c';
        if (pawns & 1) game.plateau.piece[1][0] = 'P';
        if (pawns & 2) game.plateau.piece[3][4] = 'p';
        game.penalite[0] = p1;
        game.penalite[1] = p2;
        Jeu before = game;
        int ended = pawns != 3 || p1 == 2 || p2 == 2;
        CHECK(jeu_est_termine(&game) == ended);
        if (ended) {
            int winner = p1 == 2 ? (p2 == 2 ? 0 : 2) :
                p2 == 2 ? 1 : pawns == 1 ? 1 : pawns == 2 ? 2 : 0;
            CHECK(jeu_vainqueur(&game) == winner);
        }
        CHECK(same_game(&game, &before));
    }
    /* Both have pawns, but one or both players cannot move. */
    for (int player = 1; player <= 2; ++player) {
        Jeu game = empty_game();
        game.plateau.piece[2][2] = 'P';
        game.plateau.piece[3][2] = 'p';
        CHECK(jeu_est_termine(&game) == 1);
        CHECK(jeu_vainqueur(&game) == 0);
        game.plateau.piece[player == 1 ? 0 : 4][0] = player == 1 ? 'C' : 'c';
        CHECK(jeu_est_termine(&game) == 1);
        CHECK(jeu_vainqueur(&game) == 0);
    }
    return 0;
}

typedef struct {
    const char *name;
    const char *before;
    Deplacement d1, d2;
    const char *after;
    int penalty1, penalty2;
} Scenario;

#define D(r,c,rr,cc) {{r,c},{rr,cc},{-1,-1}}
#define R(r,c,rr,cc,x,y) {{r,c},{rr,cc},{x,y}}
static const Scenario scenarios[] = {
    {"platon_forbidden_redeploy", ".P..." ".p..P" ".C.Cc" "P...c" "...p.", R(3,0,4,0,4,3), D(4,3,3,3), ".P..." ".p..P" ".C.Cc" "...pc" ".....", 0,0},
    {"forbidden_redeploy_player2", ".P..." "....p" "....." "....." "c.c..", D(0,1,1,1), R(1,4,0,4,0,1), "....." ".P..." "....." "....." "c.c..", 0,0},
    {"missing_redeploy_player1", "C...C" "....." "....p" "P...." ".....", D(3,0,4,0), D(2,4,1,4), "C...C" "....p" "....." "....." ".....", 0,0},
    {"missing_redeploy_player2", "....." "....p" "P...." "....." "c...c", D(2,0,3,0), D(1,4,0,4), "....." "....." "....." "P...." "c...c", 0,0},
    {"out_of_bounds_redeploy_player1", "C...C" "....." "....p" "P...." ".....", R(3,0,4,0,5,5), D(2,4,1,4), "C...C" "....p" "....." "....." ".....", 0,0},
    {"out_of_bounds_redeploy_player2", "....." "....p" "P...." "....." "c...c", D(2,0,3,0), R(1,4,0,4,5,5), "....." "....." "....." "P...." "c...c", 0,0},
    {"independent_moves", "CPPPC" "P...P" "....." "p...p" "cpppc", D(0,0,2,1), D(4,4,2,3), ".PPPC" "P...P" ".C.c." "p...p" "cppp.", 0,0},
    {"double_invalid", "CPPPC" "P...P" "....." "p...p" "cpppc", D(0,0,0,0), D(4,4,4,4), "CPPPC" "P...P" "....." "p...p" "cpppc", 1,1},
    {"invalid_player1", "C...." "....." "....." "....p" ".....", D(-1,0,0,0), D(3,4,2,4), "C...." "....." "....p" "....." ".....", 1,0},
    {"invalid_player2", "....." "P...." "....." "....." "....c", D(1,0,2,0), D(4,4,5,4), "....." "....." "P...." "....." "....c", 0,1},
    {"pawn_duel", "....." "..P.." "....." "..p.." ".....", D(1,2,2,2), D(3,2,2,2), "....." "....." "....." "....." ".....", 0,0},
    {"knight_duel", ".C..." "....." "....." "....." "...c.", D(0,1,2,2), D(4,3,2,2), "....." "....." "....." "....." ".....", 0,0},
    {"knight_beats_pawn", ".C..." "....." "....." "..p.." ".....", D(0,1,2,2), D(3,2,2,2), "....." "....." "..C.." "....." ".....", 0,0},
    {"pawn_loses_to_knight", "....." "..P.." "....." "....." "...c.", D(1,2,2,2), D(4,3,2,2), "....." "....." "..c.." "....." ".....", 0,0},
    {"pawn_swap", "....." ".P..." "..p.." "....." ".....", D(1,1,2,2), D(2,2,1,1), "....." ".p..." "..P.." "....." ".....", 0,0},
    {"knight_swap", ".C..." "....." "..c.." "....." ".....", D(0,1,2,2), D(2,2,0,1), ".c..." "....." "..C.." "....." ".....", 0,0},
    {"escape", "C...." "..c.." "....." "....." ".....", D(0,0,1,2), D(1,2,3,3), "....." "..C.." "....." "...c." ".....", 0,0},
    {"stationary_capture", "C...." "..p.." "....." "....p" ".....", D(0,0,1,2), D(3,4,2,4), "....." "..C.." "....p" "....." ".....", 0,0},
    {"invalid_piece_captured", "C...." "..p.." "....." "....." ".....", D(0,0,1,2), D(1,2,1,2), "....." "..C.." "....." "....." ".....", 0,1},
    {"promotion_player1", "....." "....." "....p" "P...." ".....", D(3,0,4,0), D(2,4,1,4), "....." "....p" "....." "....." "C....", 0,0},
    {"promotion_player2", "....." "....p" "P...." "....." ".....", D(2,0,3,0), D(1,4,0,4), "....c" "....." "....." "P...." ".....", 0,0},
    {"double_promotion", "....." "....p" "....." "P...." ".....", D(3,0,4,0), D(1,4,0,4), "....c" "....." "....." "....." "C....", 0,0},
    {"redeploy_player1", "C...C" "....." "....p" "P...." ".....", R(3,0,4,0,1,1), D(2,4,1,4), "C...C" ".P..p" "....." "....." ".....", 0,0},
    {"redeploy_player2", "....." "....p" "P...." "....." "c...c", D(2,0,3,0), R(1,4,0,4,3,3), "....." "....." "....." "P..p." "c...c", 0,0},
    {"double_redeploy", "C.C.." "....p" "....." "P...." "..c.c", R(3,0,4,0,2,1), R(1,4,0,4,2,3), "C.C.." "....." ".P.p." "....." "..c.c", 0,0},
    {"redeploy_collision", "C.C.." "....p" "....." "P...." "..c.c", R(3,0,4,0,2,2), R(1,4,0,4,2,2), "C.C.." "....." "....." "....." "..c.c", 0,0},
    {"redeploy_occupied", "C...C" "....." "....p" "P...." ".....", R(3,0,4,0,1,4), D(2,4,1,4), "C...C" "....p" "....." "....." ".....", 0,0},
    {"redeploy_after_knight_capture", "C...C" "..c.." "....." "P...." ".....", R(3,0,4,0,2,2), D(1,2,0,0), "c...C" "....." "....." "....." "C....", 0,0},
    {"lost_pawn_no_promotion", "....." "....p" "....." "P.c.." ".....", D(3,0,4,0), D(3,2,4,0), "....." "....p" "....." "....." "c....", 0,0},
    {"last_pawn_promoted", "....." "....." "....p" "P...." ".....", D(3,0,4,0), D(2,4,1,4), "....." "....p" "....." "....." "C....", 0,0},
    {"last_pawns_duel", "....." "..P.." "....." "..p.." ".....", D(1,2,2,2), D(3,2,2,2), "....." "....." "....." "....." ".....", 0,0},
    {"second_penalties", "....." "P...." "....." "....p" ".....", D(1,0,1,0), D(3,4,3,4), "....." "P...." "....." "....p" ".....", 1,1},
};

static int run_scenario(const Scenario *s) {
    CHECK(strlen(s->before) == 25 && strlen(s->after) == 25);
    Jeu game = empty_game();
    memcpy(game.plateau.piece, s->before, 25);
    if (!strcmp(s->name, "second_penalties")) game.penalite[0] = game.penalite[1] = 1;
    Jeu expected = game;
    memcpy(expected.plateau.piece, s->after, 25);
    expected.tour++;
    expected.penalite[0] += s->penalty1;
    expected.penalite[1] += s->penalty2;
    CHECK(jeu_resoudre_tour(&game, s->d1, s->d2) == 1);
    CHECK(same_game(&game, &expected));
    if (!strcmp(s->name, "last_pawn_promoted")) {
        CHECK(jeu_est_termine(&game) == 1);
        CHECK(jeu_vainqueur(&game) == 2);
    }
    if (!strcmp(s->name, "last_pawns_duel") || !strcmp(s->name, "second_penalties")) {
        CHECK(jeu_est_termine(&game) == 1);
        CHECK(jeu_vainqueur(&game) == 0);
    }
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    if (!strcmp(argv[1], "roundtrip") || !strcmp(argv[1], "compact_input") || !strcmp(argv[1], "turn")) {
        Jeu game;
        jeu_charger(&game);
        if (!strcmp(argv[1], "turn")) {
            Deplacement d1 = move(0,0,0,0), d2 = d1;
            if (scanf("%d%d%d%d%d%d%d%d", &d1.depart.ligne, &d1.depart.colonne,
                      &d1.arrivee.ligne, &d1.arrivee.colonne, &d2.depart.ligne,
                      &d2.depart.colonne, &d2.arrivee.ligne, &d2.arrivee.colonne) != 8) return 2;
            if (jeu_resoudre_tour(&game, d1, d2) != 1) return 1;
        }
        jeu_ecrire(&game);
        return 0;
    }
    int result;
    if (!strcmp(argv[1], "initialization")) result = initialization();
    else if (!strcmp(argv[1], "validity")) result = validity();
    else if (!strcmp(argv[1], "boundaries")) result = boundaries();
    else if (!strcmp(argv[1], "endings")) result = endings();
    else {
        for (size_t i = 0; i < sizeof scenarios / sizeof scenarios[0]; ++i) {
            if (!strcmp(argv[1], scenarios[i].name)) {
                result = run_scenario(&scenarios[i]);
                printf("%lu checks\n", checks);
                return result;
            }
        }
        fprintf(stderr, "Unknown test: %s\n", argv[1]);
        return 2;
    }
    printf("%lu checks\n", checks);
    return result;
}
