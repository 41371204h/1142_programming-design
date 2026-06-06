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

// 結局畫面的專屬計時器
static float endingTimer = 0.0f;
static bool showEndingStory = true;

static bool showHubIntro = true; // 預設為 true，一開遊戲就會觸發
static int hubDialogueIndex = 0;
static const char *hubIntroDialogue[] = {
    "Ughh... My head hurts...",
    "Elara? Joseph? Hello??? Where did everybody go...?",
    "Wait, multiple systems offline?\nAnd worse, there's oxygen leakage.",
    "There's no time to fix the whole facility.\nI have to reboot the escape system.",
    "If I can repair the three major systems,\nI can unlock the escape pod before I suffocate."
};
static int hubDialogueCount = sizeof(hubIntroDialogue) / sizeof(hubIntroDialogue[0]);

// Declare the functions only used in home page
void UpdateHub(GameState *state);
void DrawHub(GameState *state);
void UpdateGlobalInventory(GameState *state);
void DrawGlobalInventory(const GameState *state); // --- 全域物品欄繪製宣告 ---
void DrawInventoryItemDetail(const char *itemName);

// --- 跨關卡共用的 AddItem 實作 ---
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
    
    state.isLevel1Cleared = false; ///
    state.isLevel2Cleared = false;
    /*
    state.isLevel1Cleared = true;
    state.isLevel2Cleared = true;
    strcpy(state.secretSequence, "URLD"); /// for testing lv3, will be deleted after testing
    */

    // 載入全域共用的玩家 Q 版人物貼圖 (必須在 InitWindow 之後)
    state.playerSprite = LoadTexture("assets/character.png");
    // 範例：素材載入區塊 (通常在遊戲剛啟動時呼叫一次)
    state.pathSprite       = LoadTexture("assets/path.png");
    state.wallSprite       = LoadTexture("assets/wall.png");
    state.wallToolSprite   = LoadTexture("assets/wall_tool.png");
    state.wallDeviceSprite = LoadTexture("assets/wall_device.png");
    // load the key card for level 3
    state.keySprite = LoadTexture("assets/key.png");

    // 載入故事頁面用的字體：Ubuntu Sans Mono 28
    state.storyFont = LoadFontEx("assets/story_font.ttf", 28, NULL, 0);
    // --- 遊戲啟動時，先初始化第一關的變數 ---
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
                // --- 串接第一關邏輯 ---
                UpdateLevel1(&state); 
                break;
            case SCREEN_LEVEL2:
                UpdateLevel2(&state);
                break;
            case SCREEN_LEVEL3:
                UpdateLevel3(&state); // 執行第三關
                break;
            case SCREEN_ENDING:
                if (showEndingStory) {
                    endingTimer += GetFrameTime();
                    
                    if (IsKeyPressed(KEY_X)) {
                        showEndingStory = false; // 關閉故事，進入 The End
                    }
                }
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
                if (showEndingStory) {
                    const char *endingText = "Permission granted\nThe central escape pod activa%]K:(...";
                    
                    // 設定每秒顯示 15 個字母的打字機特效
                    int charsToShow = (int)(endingTimer * 15.0f); 
                    
                    // 畫出故事文字
                    DrawTextEx(state.storyFont, TextSubtext(endingText, 0, charsToShow), 
                               (Vector2){180, 400}, 56, 2, WHITE);
                               
                    // 右下角提示玩家按 X 鍵
                    DrawText("[ Press X to Continue ]", SCREEN_WIDTH / 2 - 130, SCREEN_HEIGHT - 100, 20, LIGHTGRAY);
                    
                } else {
                    // 玩家按 X 關閉故事後：顯示 The end
                    DrawText("The end", SCREEN_WIDTH / 2 - 170, SCREEN_HEIGHT / 2 - 40, 80, WHITE);
                    DrawText("[ Press ESC to escape this game ]", SCREEN_WIDTH / 2 - 190, SCREEN_HEIGHT / 2 + 80, 23, LIGHTGRAY);
                }
                break;
        }

        // --- 永遠疊加在最上層的全域物品欄繪製 ---
        if (state.inventory.opened) {
            DrawGlobalInventory(&state);
        }

        EndDrawing();
    }

    // 遊戲結束關閉前，釋放顯示卡中素材的記憶體
    UnloadTexture(state.playerSprite);
    UnloadTexture(state.pathSprite);
    UnloadTexture(state.wallSprite);
    UnloadTexture(state.wallToolSprite);
    UnloadTexture(state.wallDeviceSprite);      
    UnloadFont(state.storyFont);

    // 4. 清理並關閉
    CloseWindow();
    return 0;
}

// --------------------------------------------------------
// 主畫面 HUB 的邏輯更新
// --------------------------------------------------------
void UpdateHub(GameState *state) {

    if (showHubIntro) {
        if (IsKeyPressed(KEY_Z)) {
            hubDialogueIndex++;
            // 當對話全部看完時，關閉開場白
            if (hubDialogueIndex >= hubDialogueCount) {
                showHubIntro = false; 
            }
        }
        return; // 只要還在看故事，就直接 return，不執行下方的任何操作！
    }
    // 在 Update 層處理按鍵，就不會跟同一個幀 (Frame) 的其他畫面衝突
    if (state->isLevel1Cleared && state->isLevel2Cleared && state->isLevel3Cleared) {
        if (IsKeyPressed(KEY_SPACE)) {
            state->currentScreen = SCREEN_ENDING;
            endingTimer = 0.0f;
            showEndingStory = true;
        }
    }
    // --- 自動選取與進入關卡邏輯 ---
    if (IsKeyPressed(KEY_ENTER)) {
        if (!state->isLevel1Cleared) {
            // 第一關還沒過，按 Enter 進入第一關
            InitLevel1(); 
            state->currentScreen = SCREEN_LEVEL1;
        } 
        else if (!state->isLevel2Cleared) {
            // 第一關過了，第二關還沒過，按 Enter 進入第二關
            InitLevel2();
            state->currentScreen = SCREEN_LEVEL2;
        } 
        else if (!state->isLevel3Cleared) {
            // 前兩關都過了，第三關還沒過，按 Enter 進入第三關
            state->currentScreen = SCREEN_LEVEL3;
        }
    }
}

// --------------------------------------------------------
// 主畫面 HUB 的畫面繪製
// --------------------------------------------------------
void DrawHub(GameState *state) {
    DrawText("Main Hub", 500, 150, 56, WHITE);
    DrawText("Press ENTER to repair the highlighted system", 410, 230, 20, LIGHTGRAY);

    // 判斷目前玩家可以遊玩的關卡是哪一關
    int activeLevel = 1;
    if (state->isLevel1Cleared) activeLevel = 2;
    if (state->isLevel2Cleared) activeLevel = 3;
    if (state->isLevel3Cleared) activeLevel = 4; // 全通關

    // ---- 繪製第一台終端機 (通訊) ----
    Color color1 = state->isLevel1Cleared ? GREEN : BLUE;
    DrawRectangleRec(termLevel1, color1);
    
    if (activeLevel == 1) DrawRectangleLinesEx(termLevel1, 8.0f, GREEN);
    else DrawRectangleLinesEx(termLevel1, 3.0f, WHITE);
    
    DrawText("Communications Room", termLevel1.x - 20, termLevel1.y - 50, 28, WHITE);
    DrawText(state->isLevel1Cleared ? "[ Repaired ]" : "[ To be repaired ]", termLevel1.x + 50, termLevel1.y + 210, 20, state->isLevel1Cleared ? LIME : YELLOW);


    // ---- 繪製第二台終端機 (維修) ----
    Color color2;
    if (!state->isLevel1Cleared) color2 = DARKGRAY; 
    else color2 = state->isLevel2Cleared ? GREEN : BLUE;
    
    DrawRectangleRec(termLevel2, color2);
    
    if (activeLevel == 2) DrawRectangleLinesEx(termLevel2, 8.0f, GREEN);
    else DrawRectangleLinesEx(termLevel2, 3.0f, WHITE);

    DrawText("Archive Room", termLevel2.x + 30, termLevel2.y - 50, 28, state->isLevel1Cleared ? WHITE : GRAY);
    if (!state->isLevel1Cleared) {
        DrawText("[ System offline ]", termLevel2.x + 40, termLevel2.y + 210, 20, RED);
    } else {
        DrawText(state->isLevel2Cleared ? "[ Repaired ]" : "[ To be repaired ]", termLevel2.x + 50, termLevel2.y + 210, 20, state->isLevel2Cleared ? LIME : YELLOW);
    }


    // ---- 繪製第三台終端機 (動力核心) ----
    Color color3;
    if (!state->isLevel2Cleared) color3 = DARKGRAY; 
    else color3 = state->isLevel3Cleared ? GREEN : BLUE;

    DrawRectangleRec(termLevel3, color3);
    
    if (activeLevel == 3) DrawRectangleLinesEx(termLevel3, 8.0f, GREEN);
    else DrawRectangleLinesEx(termLevel3, 3.0f, WHITE);

    DrawText("Escape Pod", termLevel3.x + 40, termLevel3.y - 50, 28, state->isLevel2Cleared ? WHITE : GRAY);
    if (!state->isLevel2Cleared) {
        DrawText("[ Permission denied ]", termLevel3.x + 30, termLevel3.y + 210, 20, RED);
    } else {
        DrawText(state->isLevel3Cleared ? "[ Repaired ]" : "[ To be repaired ]", termLevel3.x + 50, termLevel3.y + 210, 20, state->isLevel3Cleared ? LIME : YELLOW);
    }


    // ---- 全通關結局條件 ----
    if (state->isLevel1Cleared && state->isLevel2Cleared && state->isLevel3Cleared) {
        DrawRectangle(0, 820, SCREEN_WIDTH, 140, RED);
        DrawText("All systems have been repaired! The escape pod has been unlocked!", 100, 850, 30, WHITE);
        DrawText("Press SPACE to start the launch procedure", 400, 900, 23, LIGHTGRAY);
    }

    if (showHubIntro) {
        // 畫一個半透明黑底，讓後面的終端機稍微變暗，凸顯對話框
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.6f));

        // 畫對話框底色與白框 (與前幾關的座標一致)
        DrawRectangle(150, 700, 980, 180, BLACK);
        DrawRectangleLines(150, 700, 980, 180, WHITE);

        // 使用你下載的 Ubuntu Sans Mono 故事字體
        if (hubDialogueIndex < hubDialogueCount) {
            DrawTextEx(state->storyFont, hubIntroDialogue[hubDialogueIndex], (Vector2){200, 730}, 32, 1, WHITE);
        }

        // 提示玩家按 Z 繼續
        DrawText("[Press Z]", 900, 820, 20, GRAY);
    }

}

// --- 全域物品欄邏輯更新 ---
void UpdateGlobalInventory(GameState *state) {
    // 如果目前正在查看物品詳細內容
    if (state->inventory.viewingDetail) {
        // 加入 IsKeyPressed(KEY_Z)
        if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_X)) {
            state->inventory.viewingDetail = false; // 縮回放大前的狀態
        }
        return; // 確保在詳細畫面時，不會執行到下方的上下鍵和開啟邏輯
    }

    // --- 物品欄開啟時，攔截上下鍵操作 ---
    if (IsKeyPressed(KEY_UP) && state->inventory.selected > 0) {
        state->inventory.selected--;
    }
    if (IsKeyPressed(KEY_DOWN) && state->inventory.selected < state->inventory.count - 1) {
        state->inventory.selected++;
    }

    // 按 Z 放大查看物品
    if (IsKeyPressed(KEY_Z) && state->inventory.count > 0) {
        state->inventory.viewingDetail = true;
    }
}

// --- 全域物品詳細內容繪製 ---
void DrawInventoryItemDetail(const char *itemName) {
    //DrawRectangle(240, 170, 800, 470, Fade(LIGHTGRAY, 0.97f));
    //DrawRectangleLines(240, 170, 800, 470, WHITE);

    //在這裡加各個物品的繪製程式
    if (strcmp(itemName, "Paper With Clue") == 0) {
        DrawPaperText();
    } else if (strcmp(itemName, "Morse Code Table") == 0) {
        DrawMorseTable();
    } else if (strcmp(itemName, "Navigation Command") == 0) {
        DrawNavigationCommand();
    } else if (strcmp(itemName, "Completed Map") == 0) {
        DrawCompletedMap();
    } else {
        DrawRectangle(240, 170, 800, 470, Fade(LIGHTGRAY, 0.97f));
        DrawRectangleLines(240, 170, 800, 470, WHITE);
        DrawText(itemName, 310, 220, 30, BLACK);
        DrawText("No detail available.", 310, 310, 25, DARKGRAY);
    }

    //DrawText("[ Press X to Back ]", 760, 590, 18, DARKGRAY);
}

// --- 全域物品欄介面繪製 ---
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
    // 將原本的提示文字改成這樣，讓玩家知道 Z 鍵可以切換放大/縮小
    DrawText("[ Z: View / Close ]", 740, 880, 18, GRAY);
    DrawText("[ Press C to Close ]", 920, 880, 18, GRAY);

    if (state->inventory.viewingDetail && state->inventory.count > 0) {
        DrawInventoryItemDetail(state->inventory.items[state->inventory.selected]);
    }
}
