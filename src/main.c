#include "raylib.h"
#include "game_shared.h"
#include "level1.h"
#include "level2.h"
#include "level3.h"
#include <string.h> 

#define SCREEN_HEIGHT 960
#define SCREEN_WIDTH 1280

// Homw page terminal settings
static Rectangle termLevel1 = { 150, 450, 250, 180 }; // terminal for Level 1 (left)
static Rectangle termLevel2 = { 515, 450, 250, 180 }; // terminal for Level 2 (right)
static Rectangle termLevel3 = { 880, 450, 250, 180 }; // terminal for Level 3 (middle)

// Declare the functions only used in home page
void UpdateHub(GameState *state);
void DrawHub(GameState *state);
void UpdateGlobalInventory(GameState *state);
void DrawGlobalInventory(const GameState *state); /// --- 全域物品欄繪製宣告 ---
void DrawInventoryItemDetail(const char *itemName);

/// --- 跨關卡共用的 AddItem 實作 ---
void AddItem(GameState *state, const char *itemName) {
    if (state->inventory.count < MAX_ITEMS) {
        strcpy(state->inventory.items[state->inventory.count], itemName);
        state->inventory.count++;
    }
}

int main(void) {
    // 1. Initializing the game window (res: 1280x960)
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Puzzle Game"); 
    SetTargetFPS(60);

    // 2. Initializing the game's core state 
    GameState state = {0};
    state.currentScreen = SCREEN_HUB; // the game starts at home page
    
    state.isLevel1Cleared = true; ///
    state.isLevel2Cleared = false;

    // 載入全域共用的玩家 Q 版人物貼圖 (必須在 InitWindow 之後)
    state.playerSprite = LoadTexture("assets/character.png"); // assets or resources

    /// --- 遊戲啟動時，先初始化第一關的變數 ---
    InitLevel1();
    InitLevel2();

    // 3. Game main loop
    while (!WindowShouldClose()) {
        // Cannot open inventory in main hub
        if (state.currentScreen == SCREEN_HUB || state.currentScreen == SCREEN_ENDING) {
            state.inventory.opened = false;
        }

        // Inventory can only be opened in Lv1, 2, and 3
        if (IsKeyPressed(KEY_C)) {
            if (state.currentScreen == SCREEN_LEVEL1 || 
                state.currentScreen == SCREEN_LEVEL2 ||
                state.currentScreen == SCREEN_LEVEL3){
                
                state.inventory.opened = !state.inventory.opened;
                state.inventory.viewingDetail = false;
            }
        }

        // --- A. 邏輯更新層 (Update) ---
        if (state.inventory.opened) {
            UpdateGlobalInventory(&state);
        }

        switch (state.currentScreen) {
            case SCREEN_HUB:
                UpdateHub(&state);
                break;
            case SCREEN_LEVEL1:
                /// --- 串接第一關邏輯 ---
                UpdateLevel1(&state); 
                break;
            case SCREEN_LEVEL2:
                UpdateLevel2(&state);
                break;
            case SCREEN_LEVEL3:
                UpdateLevel3(&state); // 執行第三關
                break;
            case SCREEN_ENDING:
                if (IsKeyPressed(KEY_SPACE)) state.currentScreen = SCREEN_HUB; 
                break;
            }

        // --- B. 畫面繪製層 (Draw) ---
        BeginDrawing();
        ClearBackground(BLACK); // 基礎底色

        switch (state.currentScreen) {
            case SCREEN_HUB:
                DrawHub(&state);
                break;
            case SCREEN_LEVEL1:
                // 將 &state 傳入第一關的繪製函式中
                DrawLevel1(&state); 
                break;
            case SCREEN_LEVEL2:
                DrawLevel2(&state);
                break;
            case SCREEN_LEVEL3:
                // 將 &state 傳入第三關的繪製函式中
                DrawLevel3(&state); 
                break;
            case SCREEN_ENDING:
                DrawText("Congratulations. You have completed the mission.\nThe core was repaired safely and the escape pod has been launched.", 300, 400, 35, GREEN);
                DrawText("[ Press SPACE to go bake to Main Hub ]", 480, 550, 20, GRAY);
                break;
        }

        /// --- 永遠疊加在最上層的全域物品欄繪製 ---
        if (state.inventory.opened) {
            DrawGlobalInventory(&state);
        }

        EndDrawing();
    }

    // 遊戲結束關閉前，釋放顯示卡中的玩家貼圖記憶體
    UnloadTexture(state.playerSprite);

    // 4. 清理並關閉
    CloseWindow();
    return 0;
}

// --------------------------------------------------------
// 主畫面 HUB 的邏輯更新
// --------------------------------------------------------
void UpdateHub(GameState *state) {

    Vector2 mousePos = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        
        if (CheckCollisionPointRec(mousePos, termLevel1)) {
            /// --- 確保每次點進第一關都是全新的狀態 ---
            InitLevel1(); 
            state->currentScreen = SCREEN_LEVEL1;
        }
        
        if (CheckCollisionPointRec(mousePos, termLevel2)) {
            if (state->isLevel1Cleared) {
                state->currentScreen = SCREEN_LEVEL2;
            }
        }
        
        if (CheckCollisionPointRec(mousePos, termLevel3)) {
            if (state->isLevel2Cleared) {
                state->currentScreen = SCREEN_LEVEL3;
            }
        }
    }
}

// --------------------------------------------------------
// 主畫面 HUB 的畫面繪製 (保留你的全英文介面與方塊設計)
// --------------------------------------------------------
void DrawHub(GameState *state) {
    DrawText("Main Hub", 450, 150, 40, WHITE);
    DrawText("Click on the terminal to repair the system", 480, 230, 20, LIGHTGRAY);

    Vector2 mousePos = GetMousePosition();

    // ---- 繪製第一台終端機 (通訊) ----
    bool hover1 = CheckCollisionPointRec(mousePos, termLevel1);
    Color color1 = state->isLevel1Cleared ? GREEN : (hover1 ? SKYBLUE : BLUE);
    DrawRectangleRec(termLevel1, color1);
    DrawRectangleLinesEx(termLevel1, 3, WHITE); 
    DrawText("1. Communications Section", termLevel1.x + 20, termLevel1.y + 50, 20, WHITE);
    DrawText(state->isLevel1Cleared ? "[ Repaired ]" : "[ To be repaired ]", termLevel1.x + 40, termLevel1.y + 110, 20, state->isLevel1Cleared ? LIME : YELLOW);

    // ---- 繪製第二台終端機 (維修) ----
    bool hover2 = CheckCollisionPointRec(mousePos, termLevel2);
    Color color2;
    if (!state->isLevel1Cleared) color2 = DARKGRAY; 
    else color2 = state->isLevel2Cleared ? GREEN : (hover2 ? SKYBLUE : BLUE);
    
    DrawRectangleRec(termLevel2, color2);
    DrawRectangleLinesEx(termLevel2, 3, WHITE);
    DrawText("2. Maintenance Section", termLevel2.x + 20, termLevel2.y + 50, 20, state->isLevel1Cleared ? WHITE : GRAY);
    if (!state->isLevel1Cleared) {
        DrawText("[ System offline ]", termLevel2.x + 40, termLevel2.y + 110, 20, RED);
    } else {
        DrawText(state->isLevel2Cleared ? "[ Repaired ]" : "[ To be repaired ]", termLevel2.x + 40, termLevel2.y + 110, 20, state->isLevel2Cleared ? LIME : YELLOW);
    }

    // ---- 繪製第三台終端機 (動力核心) ----
    bool hover3 = CheckCollisionPointRec(mousePos, termLevel3);
    Color color3;
    if (!state->isLevel2Cleared) color3 = DARKGRAY; 
    else color3 = state->isLevel3Cleared ? GREEN : (hover3 ? SKYBLUE : BLUE);

    DrawRectangleRec(termLevel3, color3);
    DrawRectangleLinesEx(termLevel3, 3, WHITE);
    DrawText("3. Power Section", termLevel3.x + 20, termLevel3.y + 50, 20, state->isLevel2Cleared ? WHITE : GRAY);
    if (!state->isLevel2Cleared) {
        DrawText("[ Permission denied ]", termLevel3.x + 40, termLevel3.y + 110, 20, RED);
    } else {
        DrawText(state->isLevel3Cleared ? "[ Repaired ]" : "[ To be repaired ]", termLevel3.x + 40, termLevel3.y + 110, 20, state->isLevel3Cleared ? LIME : YELLOW);
    }

    // ---- 全通關結局條件 ----
    if (state->isLevel1Cleared && state->isLevel2Cleared && state->isLevel3Cleared) {
        DrawRectangle(0, 820, SCREEN_WIDTH, 140, DARKGREEN);
        DrawText("[ Warning! ] All systems have been repaired! The escape pod has been unlocked!", 20, 850, 30, WHITE);
        DrawText("Press SPACE to start the launch procedure", 420, 900, 23, LIGHTGRAY);
        if (IsKeyPressed(KEY_SPACE)) {
            state->currentScreen = SCREEN_ENDING;
        }
    }
}

/// --- 全域物品欄邏輯更新 ---
void UpdateGlobalInventory(GameState *state) {
    if (state->inventory.viewingDetail) {
        if (IsKeyPressed(KEY_X)) {
            state->inventory.viewingDetail = false;
        }
        return;
    }

    /// --- 物品欄開啟時，攔截上下鍵操作 ---
    if (IsKeyPressed(KEY_UP) && state->inventory.selected > 0) {
        state->inventory.selected--;
    }
    if (IsKeyPressed(KEY_DOWN) && state->inventory.selected < state->inventory.count - 1) {
        state->inventory.selected++;
    }

    if (IsKeyPressed(KEY_Z) && state->inventory.count > 0) {
        state->inventory.viewingDetail = true;
    }
}

/// --- 全域物品詳細內容繪製 ---
void DrawInventoryItemDetail(const char *itemName) {
    //DrawRectangle(240, 170, 800, 470, Fade(LIGHTGRAY, 0.97f));
    //DrawRectangleLines(240, 170, 800, 470, WHITE);

    //在這裡加各個物品的繪製程式
    if (strcmp(itemName, "paper with clue") == 0) {
        DrawPaperText();
    } else if (strcmp(itemName, "Morse Code Table") == 0) {
        DrawMorseTable();
    } else if (strcmp(itemName, "Navigation Command") == 0) {
        DrawNavigationCommand();
    } else {
        DrawRectangle(240, 170, 800, 470, Fade(LIGHTGRAY, 0.97f));
        DrawRectangleLines(240, 170, 800, 470, WHITE);
        DrawText(itemName, 310, 220, 30, BLACK);
        DrawText("No detail available.", 310, 310, 25, DARKGRAY);
    }

    //DrawText("[ Press X to Back ]", 760, 590, 18, DARKGRAY);
}

/// --- 全域物品欄介面繪製 ---
void DrawGlobalInventory(const GameState *state) {
    DrawRectangle(140, 700, 1000, 220, Fade(DARKGRAY, 0.9f));
    DrawRectangleLines(140, 700, 1000, 220, WHITE);
    DrawText("--- INVENTORY ---", 170, 715, 22, LIGHTGRAY);

    if (state->inventory.count == 0) {
        DrawText("Empty...", 520, 800, 24, GRAY);
    } else {
        for (int i = 0; i < state->inventory.count; i++) {
            Color color = (i == state->inventory.selected) ? YELLOW : WHITE;
            int currentY = 755 + i * 35;
            
            if (i == state->inventory.selected) {
                DrawText("> ", 200, currentY, 25, YELLOW);
                DrawText(state->inventory.items[i], 230, currentY, 25, YELLOW);
            } else {
                DrawText(state->inventory.items[i], 230, currentY, 25, WHITE);
            }
        }
    }
    DrawText("[ Z: View ]", 780, 880, 18, GRAY);
    DrawText("[ Press C to Close ]", 920, 880, 18, GRAY);

    if (state->inventory.viewingDetail && state->inventory.count > 0) {
        DrawInventoryItemDetail(state->inventory.items[state->inventory.selected]);
    }
}
