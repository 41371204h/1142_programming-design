#include "raylib.h"
#include "level1.h"
#include "game_shared.h"
#include <string.h>
#include <stdbool.h>

#define PLAYER_SIZE 70
#define PLAYER_SPEED 4
#define MORSE_UNIT 0.3f

typedef struct {
    Rectangle rect;
} L1Player;

// ---------------------------
// 全域變數 (全部加上 static 隱藏起來)
// ---------------------------
static L1Player player;

static bool gotPaper = false;
static bool gotMorseTable = false;
static bool gotNavigationCode = false;

static bool showPaperText = false;
static bool showMorseTable = false;
static bool showDialogue = false;
static int dialogueIndex = 0;

static bool inputMode = false;
static char inputBuffer[20] = "";
static int inputLength = 0;

static int computerFlashCount = 0;
static float flashTimer = 0;
static Color computerFlashColor = GRAY;

static float gameTimer = 0;
static bool showMorseText = false;

// Morse code 閃爍相關
static int morseIndex = 0;
static float morseTimer = 0.0f;
static bool morseLightOn = true;
static const int morsePattern[] = { // 第一個數字是亮的單位數量，第二個數字是暗的單位數量
    1,1, 1,1, 3,3, // U = ..-
    1,1, 3,1, 1,3, // R = .-.
    1,1, 3,1, 1,1, 1,3, // L = .-..
    3,1, 1,1, 1, 7 // D = -..
};

static char *dialogueText =
"Press Z to interact with objects.\nPress X to cancel.\nPress C to open inventory.";

// ---------------------------
// 初始化遊戲 (原 InitGame)
// ---------------------------
void InitLevel1(void)
{
    player.rect = (Rectangle){1100, 450, PLAYER_SIZE, PLAYER_SIZE};

    // 確保每次重新進入第一關時，狀態都是乾淨的
    //currentScene = L1_SCENE_MENU;
    gotPaper = false;
    gotMorseTable = false;
    gotNavigationCode = false;

    showPaperText = false;
    showMorseTable = false;
    showDialogue = true; // 每次進入第一關都顯示一次教學對話
    dialogueIndex = 0;

    inputMode = false;
    gameTimer = 0;
    morseIndex = 0;
    morseTimer = 0.0f;
    morseLightOn = true;

    showMorseText = false;
    computerFlashColor = GRAY;
    computerFlashCount = 0;
    memset(inputBuffer, 0, sizeof(inputBuffer));
    inputLength = 0;
}

// ---------------------------
// 更新玩家移動 (完全保留)
// ---------------------------
static void UpdatePlayer()
{
    if (IsKeyDown(KEY_UP)) player.rect.y -= PLAYER_SPEED;
    if (IsKeyDown(KEY_DOWN)) player.rect.y += PLAYER_SPEED;
    if (IsKeyDown(KEY_LEFT)) player.rect.x -= PLAYER_SPEED;
    if (IsKeyDown(KEY_RIGHT)) player.rect.x += PLAYER_SPEED;
}

// ---------------------------
// 處理互動 (微調：使用全域 AddItem 與 state 切換)
// ---------------------------
static void HandleInteraction(GameState *state)
{
    Rectangle paperRect = {600, 400, 40, 40};
    Rectangle morseRect = {850, 600, 100, 120};
    Rectangle computerRect = {100, 350, 100, 120};
    Rectangle doorRect = {1150, 400, 80, 150};

    //paper & morse table
    if (CheckCollisionRecs(player.rect, paperRect) && !gotPaper) {
        showPaperText = true;
        AddItem(state, "Paper With Clue"); // 使用main_hub.c提供的 AddItem
        gotPaper = true;
    }

    if (CheckCollisionRecs(player.rect, morseRect) && !gotMorseTable) {
        showMorseTable = true;
        AddItem(state, "Morse Code Table");
        gotMorseTable = true;
    }

    
    if (showDialogue){
        if (IsKeyPressed(KEY_Z)){
            showDialogue = false;

            if (dialogueIndex == 0){
                dialogueIndex++;
                dialogueText = "URLD? I wonder what that means...";
            }
        }
        return;
    }

    //computer
    if (CheckCollisionRecs(player.rect, computerRect)) {
        inputMode = true;
        inputLength = 0;
        memset(inputBuffer, 0, sizeof(inputBuffer));
    }

    if (CheckCollisionRecs(player.rect, doorRect)) {
        state->currentScreen = SCREEN_HUB; 
    }
}

// ---------------------------
// 處理輸入框 (微調：成功時寫入過關狀態與密碼)
// ---------------------------
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
        if (strcmp(inputBuffer, "URLD") == 0 || strcmp(inputBuffer, "urld") == 0) {
            computerFlashColor = GREEN;
            computerFlashCount = 2;

            if (!gotNavigationCode) {
                AddItem(state, "Navigation Command");
                gotNavigationCode = true;
            }
            showDialogue = true;
            
            // 告訴main_hub.c第一關過了，並把密碼存起來給第三關用
            state->isLevel1Cleared = true;
            strcpy(state->secretSequence, "URLD");
        } else {
            computerFlashColor = RED;
            computerFlashCount = 2;
        }
        inputMode = false;
    }
}

// ---------------------------
// 閃爍效果
// ---------------------------
static void UpdateMorseFlash()
{
    morseTimer += GetFrameTime();

    if (morseTimer >= morsePattern[morseIndex] * MORSE_UNIT)
    {
        morseTimer = 0.0f;

        morseIndex++;

        if (morseIndex >= sizeof(morsePattern)/sizeof(morsePattern[0]))
        {
            morseIndex = 0;   // 循環播放
        }

        morseLightOn = !morseLightOn;
    }
}

// ---------------------------
// 整合：第一關邏輯總更新 (原 main 迴圈前半段)
// ---------------------------
void UpdateLevel1(GameState *state)
{
    gameTimer += GetFrameTime();

    if (state->inventory.opened) return;

    UpdatePlayer();

    if (IsKeyPressed(KEY_Z)) HandleInteraction(state);
    if (IsKeyPressed(KEY_X)) {
        showPaperText = false;
        showMorseTable = false;
        showDialogue = false;
    }
    
    // (已刪除原本的 KEY_C 物品欄邏輯，因為 main_hub.c 已經全權接管了)

    if (inputMode) HandleInputBox(state);
    if (gameTimer >= 60.0f) showMorseText = true;

    UpdateMorseFlash();
}

// ---------------------------
// 繪製紙張文字
// ---------------------------
void DrawPaperText(void)
{
    DrawRectangle(240, 250, 800, 200, LIGHTGRAY);
    DrawText("After the sound disappears,\n only light can speak", 340, 330, 30, BLACK);
}

// ---------------------------
// 繪製摩斯表
// ---------------------------
void DrawMorseTable(void)
{
    DrawRectangle(490, 80, 300, 400, LIGHTGRAY);
    DrawText("Morse Code Table\n - Placeholder", 500, 90, 25, BLACK);
}

// ---------------------------
// 繪製導航指令
// ---------------------------
void DrawNavigationCommand(void)
{
    DrawRectangle(240, 170, 800, 470, LIGHTGRAY);
    DrawText("Navigation Command", 310, 220, 30, BLACK);
    DrawText("U R L D", 310, 310, 35, BLACK);
}



// ---------------------------
// 繪製對話框
// ---------------------------
void DrawDialogue1(void)
{
    DrawRectangle(150, 700, 980, 180, BLACK);

    DrawRectangleLines(150, 700, 980, 180, WHITE);

    DrawText(dialogueText, 200, 730, 30, WHITE);

    DrawText("[Press Z]", 900, 820, 20, GRAY);
}

// ---------------------------
// 整合：第一關畫面繪製 (原 main 迴圈後半段)
// ---------------------------
void DrawLevel1(const GameState *state)
{
    DrawRectangle(1150, 400, 80, 150, BROWN); // 門
    if (!gotPaper) DrawRectangle(600, 400, 40, 40, WHITE); // 紙
    if (morseLightOn) DrawCircle(640, 700, 20, WHITE); // 摩斯閃爍點
    if (!gotMorseTable) DrawRectangle(850, 600, 100, 120, BLUE); // 摩斯表
    DrawRectangle(100, 350, 100, 120, computerFlashColor); // 電腦
    
    Rectangle sourceRec = {
        0.0f,
        0.0f,
        (float)state->playerSprite.width,
        (float)state->playerSprite.height
    };
    DrawTexturePro(state->playerSprite, sourceRec, player.rect, (Vector2){0, 0}, 0.0f, WHITE);

    if (showPaperText) {
        DrawPaperText();
    }
    if (showMorseTable) {
        DrawMorseTable();
    }
    if (showMorseText) {
        DrawText(".._   ._.   _..   ._..", 350, 50, 30, WHITE);
    }
    if (showDialogue){
        DrawDialogue1();
    }
    if (inputMode) {
        DrawRectangle(300, 800, 600, 80, WHITE);
        DrawText(inputBuffer, 330, 820, 30, BLACK);
    }
}
