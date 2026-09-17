#ifndef COMMON_H
#define COMMON_H

enum Difficulty {
    BEGINNER,
    INTERMEDIATE,
    ADVANCED
};

enum GameState {
    STATE_MENU,
    STATE_PLAYING,
    STATE_WON,
    STATE_LOST
};

/* Constantes textuelles pour l'interface et le menu principal */
const char* const STR_MENU_TITLE     = "DEMINEUR";
const char* const STR_MENU_BEGINNER  = "1. DEBUTANT (9x9)";
const char* const STR_MENU_INTERMED  = "2. INTERMEDIAIRE (16x16)";
const char* const STR_MENU_ADVANCED  = "3. AVANCE (30x16)";
const char* const STR_MENU_MARKS_ON  = "[X] MARQUES (?) (M)";
const char* const STR_MENU_MARKS_OFF = "[ ] MARQUES (?) (M)";
const char* const STR_CREDIT_LEFT    = "(C) Telev";
const char* const STR_CREDIT_RIGHT   = "Sprites: Black Squirrel";

/* Messages d'erreur système au démarrage */
const char* const STR_ERR_GRAPH_LINE1 = "=======================================================";
const char* const STR_ERR_GRAPH_LINE2 = " ERREUR : Carte graphique EGA ou VGA non detectee !";
const char* const STR_ERR_GRAPH_LINE3 = " Ce jeu necessite une carte EGA (640x350) ou VGA (640x480).";
const char* const STR_ERR_GRAPH_LINE4 = " Les cartes CGA et Hercules ne sont pas supportees.";
const char* const STR_ERR_MOUSE       = "Erreur : Pilote souris DOS non detecte (int 33h) !";



#endif
