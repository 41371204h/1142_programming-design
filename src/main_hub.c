#include "raylib.h"
#include "game_shared.h"
#include "level1.h"
#include "level2.h"
#include "level3.h"
#include "audio.h"
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
    "Ugh... My head hurts...",
    "Elara? Joseph? Hello???\nWhere did everybody go ...?",
    "Wait, multiple systems offline?\nAnd worse -- we've got an oxygen leak.",
    "There's no time to fix the whole facility.\nI have to reboot the escape system.",
    "If I can repair the three main systems,\nI can unlock the escape pod before I suffocate."
};
static int hubDialogueCount = sizeof(hubIntroDialogue) / sizeof(hubIntroDialogue[0]);

// Declare the functions only used in home page
void UpdateHub(GameState *state);
void DrawHub(GameState *state);
void UpdateGlobalInventory(GameState *state);
void DrawGlobalInventory(const GameState *state); // --- 全域物品欄繪製宣告 ---
void DrawInventoryItemDetail(const char *itemName, const GameState *state);

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

    // 初始化 SDL_mixer 音訊，並自動無限循環播放 bgm0.mp3
    init_audio();

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
    state.paperPileSprite = LoadTexture("assets/paperPile.png");
    state.puzzleParts[0] = LoadTexture("assets/part_1.png");
    state.puzzleParts[1] = LoadTexture("assets/part_2.png");
    state.puzzleParts[2] = LoadTexture("assets/part_3.png");
    state.puzzleParts[3] = LoadTexture("assets/part_4.png");
    state.puzzleParts[4] = LoadTexture("assets/part_5.png");
    state.puzzleParts[5] = LoadTexture("assets/part_6.png");
    state.puzzleParts[6] = LoadTexture("assets/part_7.png");
    state.puzzleParts[7] = LoadTexture("assets/part_8.png");
    state.puzzleParts[8] = LoadTexture("assets/part_9.png");
    state.doorSprite = LoadTexture("assets/door.png");
    state.mapSprite = LoadTexture("assets/map.png");
    state.letterSprite = LoadTexture("assets/letter.png");
    state.bookSprite   = LoadTexture("assets/book.png");
    state.codeSprite   = LoadTexture("assets/code.png");
    state.deviceSprite = LoadTexture("assets/device01.png");
    state.hubTerminalSprite = LoadTexture("assets/computer.png");
    state.bgHub    = LoadTexture("assets/background0.png"); // 主畫面
    state.bgLevel1 = LoadTexture("assets/background1.png"); // 第一關
    state.bgLevel2 = LoadTexture("assets/background2.png"); // 第二關
    state.bgLevel3 = LoadTexture("assets/background3.png"); // 第三關
    state.handleDetailSprite = LoadTexture("assets/handle.png");
    state.device02Sprite     = LoadTexture("assets/device02.png");
    

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

        // 💡 關鍵優化：當玩家主動按下 X 鍵中途想要離開關卡回到大廳時
        if (IsKeyPressed(KEY_X) && !state.inventory.opened) {
            // 如果目前是在關卡中，且沒有在看對話/大視窗，就允許退回大廳並切換回 bgm0
            if (state.currentScreen != SCREEN_HUB && state.currentScreen != SCREEN_ENDING) {
                // 這裡留給各關卡內部的關鍵對話或退出狀態機處理，
                // 為了雙重保險，我們在下方 UpdateHub 偵測 currentScreen 的改變。
            }
        }

        // --- A. 邏輯更新層 (Update) ---
        if (state.inventory.opened) {
            UpdateGlobalInventory(&state);
        }

        // 儲存更新前的畫面狀態，用來偵測畫面是否發生了切換
        int lastScreen = state.currentScreen;

        switch (state.currentScreen) {
            case SCREEN_HUB:
                UpdateHub(&state);
                break;
            case SCREEN_LEVEL1:
                UpdateLevel1(&state); 
                break;
            case SCREEN_LEVEL2:
                UpdateLevel2(&state);
                break;
            case SCREEN_LEVEL3:
                UpdateLevel3(&state); 
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

        // 💡 核心變更：全域自動 BGM 派發系統！
        // 如果這個 Frame 的狀態機改變了，代表玩家「切換了關卡」，這時才去呼叫 play_bgm
        if (state.currentScreen != lastScreen) {
            switch (state.currentScreen) {
                case SCREEN_HUB:     play_bgm(0); break; // 切回大廳，循環播 bgm0.mp3
                case SCREEN_LEVEL1:  play_bgm(1); break; // 進入第一關，循環播 bgm1.mp3
                case SCREEN_LEVEL2:  play_bgm(2); break; // 進入第二關，循環播 bgm2.mp3
                case SCREEN_LEVEL3:  play_bgm(3); break; // 進入第三關，循環播 bgm3.mp3
                case SCREEN_ENDING:  play_bgm(4); break; // 結局故事畫面，可以選擇靜音烘托氣氛
            }
        }
        if (state.currentScreen == SCREEN_LEVEL3 && IsKeyPressed(KEY_SPACE)) {
            play_bgm(3);
        }

        // --- B. 畫面繪製層 (Draw) ---
        BeginDrawing();
        ClearBackground(BLACK); // 基礎底色

        Texture2D currentBg = { 0 };

        switch (state.currentScreen) {
            case SCREEN_HUB:     currentBg = state.bgHub;    break;
            case SCREEN_LEVEL1:  currentBg = state.bgLevel1; break;
            case SCREEN_LEVEL2:  currentBg = state.bgLevel2; break;
            case SCREEN_LEVEL3:  currentBg = state.bgLevel3; break;
            case SCREEN_ENDING:  currentBg = state.bgHub;    break;
        }

        if (currentBg.id > 0) {
            Rectangle sourceRec = { 0.0f, 0.0f, (float)currentBg.width, (float)currentBg.height };
            Rectangle destRec = { 0.0f, 0.0f, (float)GetScreenWidth(), (float)GetScreenHeight() };
            DrawTexturePro(currentBg, sourceRec, destRec, (Vector2){ 0, 0 }, 0.0f, WHITE);
        }

        switch (state.currentScreen) {
            case SCREEN_HUB:
                DrawHub(&state); 
                break;
            case SCREEN_LEVEL1:
                DrawLevel1(&state); 
                break;
            case SCREEN_LEVEL2:
                DrawLevel2(&state);
                break;
            case SCREEN_LEVEL3:
                DrawLevel3(&state); 
                break;
            case SCREEN_ENDING:
                if (showEndingStory) {
                    const char *endingText = "Permission granted\nThe central escape pod activa%]K:(...";
                    int charsToShow = (int)(endingTimer * 15.0f); 
                    DrawTextEx(state.storyFont, TextSubtext(endingText, 0, charsToShow), (Vector2){180, 400}, 56, 2, WHITE);
                    DrawText("[ Press X to Continue ]", SCREEN_WIDTH / 2 - 130, SCREEN_HEIGHT - 100, 20, LIGHTGRAY);
                } else {
                    DrawText("The end", SCREEN_WIDTH / 2 - 170, SCREEN_HEIGHT / 2 - 40, 80, WHITE);
                    DrawText("[ Press ESC to escape this game ]", SCREEN_WIDTH / 2 - 190, SCREEN_HEIGHT / 2 + 80, 23, LIGHTGRAY);
                }
                break;
        }

        if (state.inventory.opened) {
            DrawGlobalInventory(&state);
        }

        EndDrawing();
    }

    // 釋放記憶體
    UnloadTexture(state.playerSprite);
    UnloadTexture(state.pathSprite);
    UnloadTexture(state.wallSprite);
    UnloadTexture(state.wallToolSprite);
    UnloadTexture(state.wallDeviceSprite);  
    UnloadTexture(state.hubTerminalSprite); 
    UnloadTexture(state.keySprite);
    UnloadTexture(state.paperPileSprite);
    for (int i = 0; i < 9; i++) {
        UnloadTexture(state.puzzleParts[i]);
    }
    UnloadTexture(state.doorSprite);
    UnloadTexture(state.mapSprite);
    UnloadTexture(state.letterSprite);
    UnloadTexture(state.bookSprite);
    UnloadTexture(state.codeSprite);
    UnloadTexture(state.deviceSprite);
    UnloadTexture(state.bgHub);
    UnloadTexture(state.bgLevel1);
    UnloadTexture(state.bgLevel2);
    UnloadTexture(state.bgLevel3);
    UnloadTexture(state.handleDetailSprite);
    UnloadTexture(state.device02Sprite);
    UnloadFont(state.storyFont);

    close_audio();
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
            if (hubDialogueIndex >= hubDialogueCount) {
                showHubIntro = false; 
            }
        }
        return; 
    }
    if (state->isLevel1Cleared && state->isLevel2Cleared && state->isLevel3Cleared) {
        if (IsKeyPressed(KEY_SPACE)) {
            state->currentScreen = SCREEN_ENDING;
            endingTimer = 0.0f;
            showEndingStory = true;
        }
    }
    if (IsKeyPressed(KEY_ENTER)) {
        if (!state->isLevel1Cleared) {
            InitLevel1(); 
            state->currentScreen = SCREEN_LEVEL1;
        } 
        else if (!state->isLevel2Cleared) {
            InitLevel2();
            state->currentScreen = SCREEN_LEVEL2;
        } 
        else if (!state->isLevel3Cleared) {
            state->currentScreen = SCREEN_LEVEL3;
        }
    }
}

// --------------------------------------------------------
// 主畫面 HUB 的畫面繪製
// --------------------------------------------------------
void DrawHub(GameState *state) {
    DrawText("Main Hub", 500, 150, 56, WHITE);
    DrawText("Press ENTER to repair the highlighted system", 320, 250, 30, LIGHTGRAY);

    int activeLevel = 1;
    if (state->isLevel1Cleared) activeLevel = 2;
    if (state->isLevel2Cleared) activeLevel = 3;
    if (state->isLevel3Cleared) activeLevel = 4; 

    Rectangle srcRec = { 0.0f, 0.0f, (float)state->hubTerminalSprite.width, (float)state->hubTerminalSprite.height };

    // ---- 1. 第一台終端機 ----
    Color tint1 = (activeLevel == 1) ? WHITE : GRAY; 
    DrawTexturePro(state->hubTerminalSprite, srcRec, termLevel1, (Vector2){0, 0}, 0.0f, tint1);
    DrawText("Communications Room", termLevel1.x + 10, termLevel1.y - 45, 28, WHITE);
    DrawText(state->isLevel1Cleared ? "[ Repaired ]" : "[ To be repaired ]", 
             termLevel1.x + 50, termLevel1.y + termLevel1.height + 20, 
             20, state->isLevel1Cleared ? LIME : YELLOW);

    // ---- 2. 第二台終端機 ----
    Color tint2 = (!state->isLevel1Cleared) ? Fade(WHITE, 0.3f) : ((activeLevel == 2) ? WHITE : GRAY);
    DrawTexturePro(state->hubTerminalSprite, srcRec, termLevel2, (Vector2){0, 0}, 0.0f, tint2);
    Color textTitleColor2 = state->isLevel1Cleared ? WHITE : Fade(WHITE, 0.3f);
    DrawText("Archive Room", termLevel2.x + 60, termLevel2.y - 45, 28, textTitleColor2);
    if (!state->isLevel1Cleared) {
        DrawText("[ System offline ]", termLevel2.x + 50, termLevel2.y + termLevel2.height + 20, 20, Fade(RED, 0.5f));
    } else {
        DrawText(state->isLevel2Cleared ? "[ Repaired ]" : "[ To be repaired ]", 
                 termLevel2.x + 50, termLevel2.y + termLevel2.height + 20, 20, state->isLevel2Cleared ? LIME : YELLOW);
    }

    // ---- 3. 第三台終端機 ----
    Color tint3 = (!state->isLevel2Cleared) ? Fade(WHITE, 0.3f) : ((activeLevel == 3) ? WHITE : GRAY);
    DrawTexturePro(state->hubTerminalSprite, srcRec, termLevel3, (Vector2){0, 0}, 0.0f, tint3);
    Color textTitleColor3 = state->isLevel2Cleared ? WHITE : Fade(WHITE, 0.3f);
    DrawText("Escape Pod", termLevel3.x + 70, termLevel3.y - 45, 28, textTitleColor3);
    if (!state->isLevel2Cleared) {
        DrawText("[ Permission denied ]", termLevel3.x + 40, termLevel3.y + termLevel3.height + 20, 20, Fade(RED, 0.5f));
    } else {
        DrawText(state->isLevel3Cleared ? "[ Repaired ]" : "[ To be repaired ]", 
                 termLevel3.x + 50, termLevel3.y + termLevel3.height + 20, 20, state->isLevel3Cleared ? LIME : YELLOW);
    }

    if (state->isLevel1Cleared && state->isLevel2Cleared && state->isLevel3Cleared) {
        DrawRectangle(0, 820, SCREEN_WIDTH, 140, RED);
        DrawText("All systems have been repaired! The escape pod has been unlocked!", 100, 850, 30, WHITE);
        DrawText("Press SPACE to start the launch procedure", 400, 900, 23, LIGHTGRAY);
    }

    if (showHubIntro) {
        DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.5f)); 
        DrawRectangle(150, 700, 980, 180, BLACK);
        DrawRectangleLines(150, 700, 980, 180, WHITE);
        if (hubDialogueIndex < hubDialogueCount) {
            DrawTextEx(state->storyFont, hubIntroDialogue[hubDialogueIndex], (Vector2){200, 730}, 32, 1, WHITE);
        }
        DrawText("[Press Z to Continue]", 850, 820, 20, GRAY);
    }
}

void UpdateGlobalInventory(GameState *state) {
    if (state->inventory.viewingDetail) {
        if (IsKeyPressed(KEY_Z) || IsKeyPressed(KEY_X)) {
            state->inventory.viewingDetail = false; 
        }
        return; 
    }
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

void DrawInventoryItemDetail(const char *itemName, const GameState *state) { 
    if (strcmp(itemName, "Paper With Clue") == 0) {
        DrawPaperText();
    } else if (strcmp(itemName, "Morse Code Table") == 0) {
        DrawMorseTable(state);
    } else if (strcmp(itemName, "Navigation Command") == 0) {
        DrawNavigationCommand();
    } else if (strcmp(itemName, "Completed Map") == 0) {
        DrawCompletedMap(state); 
    } else if (strcmp(itemName, "Handle") == 0) {
        // 畫出大詳細資訊底框 (半透明灰色增加高級感)
        DrawRectangle(390, 200, 500, 450, Fade(LIGHTGRAY, 0.95f));
        DrawRectangleLines(390, 200, 500, 450, DARKGRAY);
        
        DrawText("Item: Maintenance Handle", 440, 240, 28, BLACK);
        
        // 繪製專屬的 handle.png (置中並放大成 200x200 的規格顯示)
        Rectangle srcRec = { 0.0f, 0.0f, (float)state->handleDetailSprite.width, (float)state->handleDetailSprite.height };
        Rectangle destRec = { 540, 300, 200, 200 }; 
        DrawTexturePro(state->handleDetailSprite, srcRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);
    } else {
        DrawRectangle(240, 170, 800, 470, Fade(LIGHTGRAY, 0.97f));
        DrawRectangleLines(240, 170, 800, 470, WHITE);
        DrawText(itemName, 310, 220, 30, BLACK);
        DrawText("No detail available.", 310, 310, 25, DARKGRAY);
    }
}

void DrawGlobalInventory(const GameState *state) {
    DrawRectangle(140, 700, 1000, 220, Fade(DARKGRAY, 0.9f));
    DrawRectangleLines(140, 700, 1000, 220, WHITE);
    DrawText("--- INVENTORY ---", 170, 715, 22, LIGHTGRAY);

    if (state->inventory.count == 0) {
        DrawText("Empty...", 520, 800, 24, GRAY);
    } else {
        for (int i = 0; i < state->inventory.count; i++) {
            int col = i / 4;
            int row = i % 4;
            int currentX = 230 + (col * 400); 
            int currentY = 755 + (row * 35);  
            
            if (i == state->inventory.selected) {
                DrawText("> ", currentX - 30, currentY, 25, YELLOW);
                DrawText(state->inventory.items[i], currentX, currentY, 25, YELLOW);
            } else {
                DrawText(state->inventory.items[i], currentX, currentY, 25, WHITE);
            }
        }
    }
    DrawText("[ Z: View / Close ]", 740, 880, 18, GRAY);
    DrawText("[ Press C to Close ]", 920, 880, 18, GRAY);

    if (state->inventory.viewingDetail && state->inventory.count > 0) {
        DrawInventoryItemDetail(state->inventory.items[state->inventory.selected], state);
    }
}