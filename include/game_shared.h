// include/game_shared.h
#ifndef GAME_SHARED_H
#define GAME_SHARED_H

#include "raylib.h"

#define MAX_ITEMS 10

// 遊戲場景列舉
typedef enum {
    SCREEN_HUB,
    SCREEN_LEVEL1,
    SCREEN_LEVEL2,
    SCREEN_LEVEL3,
    SCREEN_ENDING
} GameScreen;

// 物品欄
typedef struct {
    char items[MAX_ITEMS][100];
    int count;
    bool opened;
    int selected;
} Inventory;

// Globally shared game state
typedef struct {
    GameScreen currentScreen;
    bool isLevel1Cleared;
    bool isLevel2Cleared;
    bool isLevel3Cleared;
    char secretSequence[5];
    
    // Globally shared inventory variable
    Inventory inventory; 
    // Globally shared variable that store the apperence of the character
    Texture2D playerSprite;
} GameState;

// add item to inventory
void AddItem(GameState *state, const char *itemName);

#endif // GAME_SHARED_H