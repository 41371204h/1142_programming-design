#include "raylib.h"
#include "level1.h"
#include "game_shared.h"
#include <string.h>
#include <stdbool.h>

#define PLAYER_SIZE 40
#define PLAYER_SPEED 4

// --- 加上 L1_ 前綴防止跟別關撞名 ---
typedef enum {
    L1_SCENE_MENU,
    L1_SCENE_LEVEL1
} L1Scene;

typedef struct {
    Rectangle rect;
} L1Player;

// =============================
// 全域變數 (全部加上 static 隱藏起來)
// =============================
static L1Scene currentScene = L1_SCENE_MENU;
static L1Player player;

static bool gotPaper = false;
static bool gotMorseTable = false;
static bool gotNavigationCode = false;

static bool showPaperText = false;
static bool showMorseTable = false;

static bool inputMode = false;
static char inputBuffer[20] = "";
static int inputLength = 0;

static int computerFlashCount = 0;
static float flashTimer = 0;
static Color computerFlashColor = GRAY;

static float gameTimer = 0;
static bool showMorseText = false;

static char menuText[100] = "main menu\npress Z to start";

// =============================
// 初始化遊戲 (原 InitGame)
// =============================
void InitLevel1(void)
{
    player.rect = (Rectangle){1100, 450, PLAYER_SIZE, PLAYER_SIZE};

    // 確保每次重新進入第一關時，狀態都是乾淨的
    currentScene = L1_SCENE_MENU;
    gotPaper = false;
    gotMorseTable = false;
    gotNavigationCode = false;
    showPaperText = false;
    showMorseTable = false;
    inputMode = false;
    gameTimer = 0;
    showMorseText = false;
    computerFlashColor = GRAY;
    computerFlashCount = 0;
    strcpy(menuText, "main menu\npress Z to start");
    memset(inputBuffer, 0, sizeof(inputBuffer));
    inputLength = 0;
}

// =============================
// 更新玩家移動 (完全保留)
// =============================
static void UpdatePlayer()
{
    if (IsKeyDown(KEY_UP)) player.rect.y -= PLAYER_SPEED;
    if (IsKeyDown(KEY_DOWN)) player.rect.y += PLAYER_SPEED;
    if (IsKeyDown(KEY_LEFT)) player.rect.x -= PLAYER_SPEED;
    if (IsKeyDown(KEY_RIGHT)) player.rect.x += PLAYER_SPEED;
}

// =============================
// 處理互動 (微調：使用全域 AddItem 與 state 切換)
// =============================
static void HandleInteraction(GameState *state)
{
    Rectangle paperRect = {600, 400, 40, 40};
    Rectangle morseRect = {850, 600, 100, 120};
    Rectangle computerRect = {100, 350, 100, 120};
    Rectangle doorRect = {1150, 400, 80, 150};

    if (CheckCollisionRecs(player.rect, paperRect) && !gotPaper) {
        showPaperText = true;
        AddItem(state, "paper with clue"); // 使用main_hub.c提供的 AddItem
        gotPaper = true;
    }

    if (CheckCollisionRecs(player.rect, morseRect) && !gotMorseTable) {
        showMorseTable = true;
        AddItem(state, "Morse Code Table");
        gotMorseTable = true;
    }

    if (CheckCollisionRecs(player.rect, computerRect)) {
        inputMode = true;
        inputLength = 0;
        memset(inputBuffer, 0, sizeof(inputBuffer));
    }

    if (CheckCollisionRecs(player.rect, doorRect)) {
        // 原本是改文字，我們直接讓他回到主畫面
        state->currentScreen = SCREEN_HUB; 
    }
}

// =============================
// 處理輸入框 (微調：成功時寫入過關狀態與密碼)
// =============================
static void HandleInputBox(GameState *state)
{
    int key = GetCharPressed();

    while (key > 0) {
        if ((key >= 32) && (key <= 125) && (inputLength < 10)) {
            inputBuffer[inputLength] = (char)key;
            inputLength++;
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && inputLength > 0) {
        inputLength--;
        inputBuffer[inputLength] = '\0';
    }

    if (IsKeyPressed(KEY_ENTER)) { ///
        if (strcmp(inputBuffer, "URDL") == 0) {
            computerFlashColor = GREEN;
            computerFlashCount = 2;

            if (!gotNavigationCode) {
                AddItem(state, "Navigation Command: U R D L");
                gotNavigationCode = true;
            }
            
            // 告訴main_hub.c第一關過了，並把密碼存起來給第三關用
            state->isLevel1Cleared = true;
            strcpy(state->secretSequence, "URDL"); 
            strcpy(menuText, "Main Menu\n Level 1 Completed"); // 更新內部選單文字
        } else {
            computerFlashColor = RED;
            computerFlashCount = 2;
        }
        inputMode = false;
    }
}

// =============================
// 閃爍效果
// =============================
static void UpdateComputerFlash()
{
    if (computerFlashCount > 0) {
        flashTimer += GetFrameTime();
        if (flashTimer >= 0.3f) {
            flashTimer = 0;
            computerFlashCount--;
            if (computerFlashCount == 0) {
                computerFlashColor = GRAY;
            }
        }
    }
}

// =============================
// 整合：第一關邏輯總更新 (原 main 迴圈前半段)
// =============================
void UpdateLevel1(GameState *state)
{
    if (currentScene == L1_SCENE_MENU) {
        if (IsKeyPressed(KEY_Z)) currentScene = L1_SCENE_LEVEL1;
    }
    else if (currentScene == L1_SCENE_LEVEL1) {
        gameTimer += GetFrameTime();

        if (state->inventory.opened) return;

        UpdatePlayer();

        if (IsKeyPressed(KEY_Z)) HandleInteraction(state);
        if (IsKeyPressed(KEY_X)) {
            showPaperText = false;
            showMorseTable = false;
        }
        
        // (已刪除原本的 KEY_C 物品欄邏輯，因為 main_hub.c 已經全權接管了)

        if (inputMode) HandleInputBox(state);
        if (gameTimer >= 60.0f) showMorseText = true;

        UpdateComputerFlash();
    }
}

// =============================
// 整合：第一關畫面繪製 (原 main 迴圈後半段)
// =============================
void DrawLevel1(const GameState *state)
{
    if (currentScene == L1_SCENE_MENU) {
        DrawText(menuText, 400, 450, 40, WHITE);
    }
    else if (currentScene == L1_SCENE_LEVEL1) {
        DrawRectangle(1150, 400, 80, 150, BROWN); // 門
        if (!gotPaper) DrawRectangle(600, 400, 40, 40, WHITE); // 紙
        if (((int)(GetTime() * 4) % 2 == 0)) DrawCircle(640, 700, 20, WHITE); // 摩斯閃爍點
        if (!gotMorseTable) DrawRectangle(850, 600, 100, 120, BLUE); // 摩斯表
        DrawRectangle(100, 350, 100, 120, computerFlashColor); // 電腦
        
        // 將原本畫紅方塊的地方，換成assets/character.png
        Rectangle sourceRec = { 0.0f, 0.0f, (float)state->playerSprite.width, (float)state->playerSprite.height };
        DrawTexturePro(state->playerSprite, sourceRec, player.rect, (Vector2){0, 0}, 0.0f, WHITE);

        if (showPaperText) {
            DrawRectangle(250, 250, 800, 200, LIGHTGRAY);
            DrawText("After the sound disappears,\n only light can speak", 350, 330, 30, BLACK);
        }
        if (showMorseTable) {
            DrawRectangle(850, 150, 300, 400, LIGHTGRAY);
            DrawText("Morse Code Table\n - Placeholder", 880, 300, 25, BLACK);
        }
        if (showMorseText) {
            DrawText(".._   ._.   _..   ._..", 350, 50, 30, WHITE);
        }
        if (inputMode) {
            DrawRectangle(300, 800, 600, 80, WHITE);
            DrawText(inputBuffer, 330, 820, 30, BLACK);
        }
    }
}