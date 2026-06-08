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
    bool viewingDetail;
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
    Texture2D pathSprite;
    Texture2D wallSprite;
    Texture2D wallToolSprite;
    Texture2D wallDeviceSprite;
    Texture2D keySprite;
    Texture2D paperPileSprite;      
    Texture2D puzzleParts[9];
    Texture2D doorSprite;
    Texture2D mapSprite;
    Texture2D letterSprite;    // letter.png
    Texture2D bookSprite;      // book.png
    Texture2D codeSprite;      // code.png
    Texture2D deviceSprite;    // device01.png
    Texture2D hubTerminalSprite;
    Texture2D bgHub;
    Texture2D bgLevel1;
    Texture2D bgLevel2;
    Texture2D bgLevel3;
    Font storyFont;
} GameState;

// add item to inventory
void AddItem(GameState *state, const char *itemName);

#endif // GAME_SHARED_H