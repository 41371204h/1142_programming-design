// include/game_shared.h
#ifndef GAME_SHARED_H
#define GAME_SHARED_H

#include "raylib.h"
#include <stdbool.h>

typedef enum { 
    SCREEN_HUB = 0, // The hub screen where players can choose levels
    SCREEN_LEVEL1, // The first level of the game
    SCREEN_LEVEL2, // The second level of the game
    SCREEN_LEVEL3, // The third level of the game
    SCREEN_ENDING  // The ending screen after completing all levels
} GameScreen;

typedef struct {
    GameScreen currentScreen;
    bool isLevel1Cleared; // Indicates if Level 1 has been cleared
    bool isLevel2Cleared; // Indicates if Level 2 has been cleared
    bool isLevel3Cleared; // Indicates if Level 3 has been cleared
    char secretSequence[5]; // Stores the secret sequence for unlocking Level 3
} GameState;

#endif