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
// 全域變數
// ---------------------------
static L1Player player;

static bool gotPaper = false;
static bool gotMorseTable = false;
static bool gotNavigationCode = false;

static bool showPaperText = false;
static bool showMorseTable = false;
static bool showNavigationCommand = false;
static bool showDialogue = false;
static bool returnToHubAfterDialogue = false;
static int dialogueIndex = 0;

static bool inputMode = false;
static char inputBuffer[20] = "";
static int inputLength = 0;

static int computerFlashCount = 0;
static float flashTimer = 0;
static Color computerFlashColor = GRAY;
static Color computerFlashTargetColor = GRAY;
static bool computerFlashOn = false;

static float gameTimer = 0;
static bool showMorseText = false;

typedef enum {
    L1_DIALOGUE_START,
    L1_DIALOGUE_PAPER,
    L1_DIALOGUE_MORSE_TABLE,
    L1_DIALOGUE_SUCCESS
} L1DialogueMode;

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

static const char **dialogueLines = NULL;
static int dialogueCount = 0;
static L1DialogueMode dialogueMode = L1_DIALOGUE_START;

static const char *initialDialogue[] = {
    "Anyway, I need to figure out the situation first.",
    "Maybe there are some messages left by others\nin the communications record room.",
    "...Wait.\nWhy is this place such a mess?",
    "Oh god, those data have all been destroyed!",
    "And the voice recording cannot be played back either...\nWhat should I do now?",
    "(Press Z to interact with objects.\nPress X to cancel.\nPress C to open inventory.)"
};

static const char *paperDialogue[] = {
    "A note!",
    "After the sound disappears, only light can speak...",
    "It looks like I should watch the light\nwhen the sound is gone.",
    "What could it mean?\nMaybe it's related to the flashing light in the room?"
};

static const char *morseTableDialogue[] = {
    "A Morse code table...?",
    "What is it for?"
};

static const char *doorLockedDialogue[] = {
    "I need to get the information first."
};

static const char *successDialogue[] = {
    "A string of gibberish...",
    "Looks like they are all composed of U, D, L, R.",
    "What could it mean?\nWhat ever it is, it must be important."
};

static void StartDialogue(const char **lines, int count, L1DialogueMode mode)
{
    dialogueLines = lines;
    dialogueCount = count;
    dialogueIndex = 0;
    dialogueMode = mode;
    showDialogue = true;
}

static void FinishDialogue(GameState *state)
{
    showDialogue = false;

    switch (dialogueMode) {
        case L1_DIALOGUE_PAPER:
            showPaperText = false;
            break;
        case L1_DIALOGUE_MORSE_TABLE:
            showMorseTable = false;
            break;
        case L1_DIALOGUE_SUCCESS:
            showNavigationCommand = false;
            state->currentScreen = SCREEN_HUB;
            break;
        default:
            break;
    }

    if (returnToHubAfterDialogue) {
        returnToHubAfterDialogue = false;
        state->currentScreen = SCREEN_HUB;
    }

    dialogueMode = L1_DIALOGUE_START;
}

static void AdvanceDialogue(GameState *state)
{
    if (!IsKeyPressed(KEY_Z)) return;

    dialogueIndex++;
    if (dialogueIndex >= dialogueCount) {
        FinishDialogue(state);
    }
}

static void ClearPendingTextInput(void)
{
    while (GetCharPressed() > 0) {
    }
}
// ---------------------------
// 初始化第一關
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
    showNavigationCommand = false;
    showDialogue = false;
    returnToHubAfterDialogue = false;
    StartDialogue(initialDialogue, sizeof(initialDialogue) / sizeof(initialDialogue[0]), L1_DIALOGUE_START);

    inputMode = false;
    gameTimer = 0;
    morseIndex = 0;
    morseTimer = 0.0f;
    morseLightOn = true;

    showMorseText = false;
    computerFlashColor = GRAY;
    computerFlashTargetColor = GRAY;
    computerFlashCount = 0;
    computerFlashOn = false;
    flashTimer = 0;
    memset(inputBuffer, 0, sizeof(inputBuffer));
    inputLength = 0;
}

// ---------------------------
// 更新玩家移動
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

    // paper
    if (CheckCollisionRecs(player.rect, paperRect) && !gotPaper) {
        showPaperText = true;
        AddItem(state, "Paper With Clue"); // 使用main提供的 AddItem
        gotPaper = true;
        StartDialogue(paperDialogue, sizeof(paperDialogue) / sizeof(paperDialogue[0]), L1_DIALOGUE_PAPER);
        return;
    }

    // Morse table
    if (CheckCollisionRecs(player.rect, morseRect) && !gotMorseTable) {
        showMorseTable = true;
        AddItem(state, "Morse Code Table");
        gotMorseTable = true;
        StartDialogue(morseTableDialogue, sizeof(morseTableDialogue) / sizeof(morseTableDialogue[0]), L1_DIALOGUE_MORSE_TABLE);
        return;
    }

    
    if (showDialogue){
        AdvanceDialogue(state);
        return;
    }

    //computer
    if (CheckCollisionRecs(player.rect, computerRect)) {
        inputMode = true;
        inputLength = 0;
        memset(inputBuffer, 0, sizeof(inputBuffer));
        ClearPendingTextInput();
    }

    // door
    if (CheckCollisionRecs(player.rect, doorRect)) {
        StartDialogue(doorLockedDialogue, sizeof(doorLockedDialogue) / sizeof(doorLockedDialogue[0]), L1_DIALOGUE_START);
        returnToHubAfterDialogue = false;
    }
}

// ---------------------------
// 處理輸入框 (微調：成功時寫入過關狀態與密碼)
// ---------------------------
static void HandleInputBox(GameState *state)
{
    int key = GetCharPressed();

    while (key > 0) {
        if ((key == 'c') || (key == 'C') || (key == 'z') || (key == 'Z')) {
            key = GetCharPressed();
            continue;
        }

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
            computerFlashTargetColor = GREEN;
            computerFlashColor = computerFlashTargetColor;
            computerFlashCount = 2;
            computerFlashOn = true;
            flashTimer = 0;

            if (!gotNavigationCode) {
                AddItem(state, "Navigation Command");
                gotNavigationCode = true;
            }
            showNavigationCommand = true;
            StartDialogue(successDialogue, sizeof(successDialogue) / sizeof(successDialogue[0]), L1_DIALOGUE_SUCCESS);
            returnToHubAfterDialogue = true;
            
            // 告訴main_hub.c第一關過了，並把密碼存起來給第三關用
            state->isLevel1Cleared = true;
            strcpy(state->secretSequence, "URLD");
        } else {
            computerFlashTargetColor = RED;
            computerFlashColor = computerFlashTargetColor;
            computerFlashCount = 2;
            computerFlashOn = true;
            flashTimer = 0;
        }
        inputMode = false;
    }
}

// ---------------------------
// 摩斯密碼閃爍
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
            morseIndex = 0; // 循環播放
        }

        morseLightOn = !morseLightOn;
    }
}

// ---------------------------
// 電腦閃爍
// ---------------------------
static void UpdateComputerFlash()
{
    if (computerFlashCount <= 0) return;

    flashTimer += GetFrameTime();

    if (flashTimer >= 0.18f) {
        flashTimer = 0;

        if (computerFlashOn) {
            computerFlashColor = GRAY;
            computerFlashOn = false;
            computerFlashCount--;
        } else if (computerFlashCount > 0) {
            computerFlashColor = computerFlashTargetColor;
            computerFlashOn = true;
        }
    }
}

// ---------------------------
// 整合：第一關邏輯總更新
// ---------------------------
void UpdateLevel1(GameState *state)
{
    gameTimer += GetFrameTime();

    if (state->inventory.opened) return;

    if (showDialogue) { // 這整段不知道要幹嘛
        AdvanceDialogue(state);
        UpdateMorseFlash();
        UpdateComputerFlash();
        return;
    }

    UpdatePlayer();

    bool startInput = false;

    if (!inputMode && IsKeyPressed(KEY_Z)) {
        HandleInteraction(state);
        startInput = inputMode;
    }

    if (IsKeyPressed(KEY_X)) {
        if (showDialogue && returnToHubAfterDialogue) {
            returnToHubAfterDialogue = false;
            showDialogue = false;
            state->currentScreen = SCREEN_HUB;
            return;
        }
        showPaperText = false;
        showMorseTable = false;
        showNavigationCommand = false;
        showDialogue = false;
        dialogueMode = L1_DIALOGUE_START;
    }
    
    // (已刪除原本的 KEY_C 物品欄邏輯，因為 main_hub.c 已經全權接管了)

    if (inputMode && !startInput) HandleInputBox(state);
    if (gameTimer >= 60.0f) showMorseText = true;

    UpdateMorseFlash();
    UpdateComputerFlash();
}

// ---------------------------
// 繪製紙張文字
// ---------------------------
void DrawPaperText(void)
{
    DrawRectangle(240, 250, 800, 200, LIGHTGRAY);
    DrawText("After the sound disappears,\nonly light can speak.", 340, 330, 30, BLACK);
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
    DrawText("UUUU RRR D\nUU RR DDDD LLL D", 310, 310, 35, BLACK);
    DrawText("U R L D", 310, 500, 50, BLACK);
    DrawLineEx((Vector2){300, 550}, (Vector2){510, 550}, 5.0f, RED);
}

// ---------------------------
// 繪製對話框
// ---------------------------
void DrawDialogue1(const GameState *state)
{
    DrawRectangle(150, 700, 980, 180, BLACK);

    DrawRectangleLines(150, 700, 980, 180, WHITE);

    if (dialogueLines != NULL && dialogueIndex < dialogueCount) {
        DrawTextEx(state->storyFont, dialogueLines[dialogueIndex], (Vector2){200, 730}, 32, 1, WHITE);
    }

    DrawText("[Press Z to Continue]", 850, 820, 20, GRAY);
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
    if (showNavigationCommand) {
        DrawNavigationCommand();
    }
    if (showMorseText) {
        DrawText(".._   ._.   ._..   _..", 350, 50, 30, WHITE); // U R L D
    }
    if (showDialogue){
        DrawDialogue1(state);
    }
    if (inputMode) {
        DrawRectangle(300, 800, 600, 80, WHITE);
        DrawText(inputBuffer, 330, 820, 30, BLACK);
    }
}
