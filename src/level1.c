// TODO: add a rectangle that shows the player gets the key after entering the right answer

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
static Color computerFlashColor = WHITE; 
static Color computerFlashTargetColor = WHITE;
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
static const int morsePattern[] = { 
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
    "After the sound disappears,\nonly light can speak...",
    "It looks like I should watch the light\nwhen the sound is gone.",
    "What could it mean?\nMaybe it's related to the flashing light in the room?"
};

static const char *morseTableDialogue[] = {
    "A Morse code table...?",
    "What is it for?"
};

// 💡 恢復：門被鎖住的對話內容
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

    // 💡 恢復：看完對話後切換畫面的判斷
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

void InitLevel1(void)
{
    player.rect = (Rectangle){1100, 450, PLAYER_SIZE, PLAYER_SIZE};

    gotPaper = false;
    gotMorseTable = false;
    gotNavigationCode = false;

    showPaperText = false;
    showMorseTable = false;
    showNavigationCommand = false;
    showDialogue = false;
    returnToHubAfterDialogue = false; // 💡 恢復初始化
    StartDialogue(initialDialogue, sizeof(initialDialogue) / sizeof(initialDialogue[0]), L1_DIALOGUE_START);

    inputMode = false;
    gameTimer = 0;
    morseIndex = 0;
    morseTimer = 0.0f;
    morseLightOn = true;

    showMorseText = false;
    computerFlashColor = WHITE; 
    computerFlashTargetColor = WHITE;
    computerFlashCount = 0;
    computerFlashOn = false;
    flashTimer = 0;
    memset(inputBuffer, 0, sizeof(inputBuffer));
    inputLength = 0;
}

static void UpdatePlayer()
{
    if (IsKeyDown(KEY_UP)) player.rect.y -= PLAYER_SPEED;
    if (IsKeyDown(KEY_DOWN)) player.rect.y += PLAYER_SPEED;
    if (IsKeyDown(KEY_LEFT)) player.rect.x -= PLAYER_SPEED;
    if (IsKeyDown(KEY_RIGHT)) player.rect.x += PLAYER_SPEED;

    if (player.rect.x < 0) player.rect.x = 0;
    if (player.rect.y < 0) player.rect.y = 0;
    if (player.rect.x > GetScreenWidth() - player.rect.width) player.rect.x = (float)GetScreenWidth() - player.rect.width;
    if (player.rect.y > GetScreenHeight() - player.rect.height) player.rect.y = (float)GetScreenHeight() - player.rect.height;
}

static void HandleInteraction(GameState *state)
{
    Rectangle paperRect = {530, 360, 150, 100};      
    Rectangle morseRect = {850, 650, 200, 100};       
    // 💡 3. 左邊裝置往右移一點，同步更新物理碰撞判定盒 (X 從 120 改為 180)
    Rectangle computerRect = {180, 350, 120, 140};  
    // 💡 恢復：門的隱形判定區域（坐標與原本位置一致）
    Rectangle doorRect = {1080, 375, 200, 200};     

    if (CheckCollisionRecs(player.rect, paperRect) && !gotPaper) {
        showPaperText = true;
        AddItem(state, "Paper With Clue"); 
        gotPaper = true;
        StartDialogue(paperDialogue, sizeof(paperDialogue) / sizeof(paperDialogue[0]), L1_DIALOGUE_PAPER);
        return;
    }

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

    if (CheckCollisionRecs(player.rect, computerRect)) {
        inputMode = true;
        inputLength = 0;
        memset(inputBuffer, 0, sizeof(inputBuffer));
        ClearPendingTextInput();
    }

    // 💡 恢復：走到隱形門的區域按 Z，觸發被鎖住的對話
    if (CheckCollisionRecs(player.rect, doorRect)) {
        StartDialogue(doorLockedDialogue, sizeof(doorLockedDialogue) / sizeof(doorLockedDialogue[0]), L1_DIALOGUE_START);
        returnToHubAfterDialogue = false;
    }
}

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

    if (IsKeyPressed(KEY_ENTER)) { 
        if (strcmp(inputBuffer, "URLD") == 0 || strcmp(inputBuffer, "urld") == 0) {
            computerFlashTargetColor = Fade(GREEN, 0.6f); 
            computerFlashColor = computerFlashTargetColor;
            computerFlashCount = 3; 
            computerFlashOn = true;
            flashTimer = 0;

            if (!gotNavigationCode) {
                AddItem(state, "Navigation Command");
                gotNavigationCode = true;
            }
            showNavigationCommand = true;
            StartDialogue(successDialogue, sizeof(successDialogue) / sizeof(successDialogue[0]), L1_DIALOGUE_SUCCESS);
            returnToHubAfterDialogue = true; // 💡 恢復
            
            state->isLevel1Cleared = true;
            strcpy(state->secretSequence, "URLD");
        } else {
            computerFlashTargetColor = Fade(RED, 0.6f); 
            computerFlashColor = computerFlashTargetColor;
            computerFlashCount = 3;
            computerFlashOn = true;
            flashTimer = 0;
        }
        inputMode = false;
    }
}

static void UpdateMorseFlash()
{
    morseTimer += GetFrameTime();

    if (morseTimer >= morsePattern[morseIndex] * MORSE_UNIT)
    {
        morseTimer = 0.0f;
        morseIndex++;

        if (morseIndex >= sizeof(morsePattern)/sizeof(morsePattern[0]))
        {
            morseIndex = 0; 
        }

        morseLightOn = !morseLightOn;
    }
}

static void UpdateComputerFlash()
{
    if (computerFlashCount <= 0) return;

    flashTimer += GetFrameTime();

    if (flashTimer >= 0.15f) {
        flashTimer = 0;

        if (computerFlashOn) {
            computerFlashColor = WHITE; 
            computerFlashOn = false;
            computerFlashCount--;
        } else if (computerFlashCount > 0) {
            computerFlashColor = computerFlashTargetColor; 
            computerFlashOn = true;
        }
    }
}

void UpdateLevel1(GameState *state)
{
    gameTimer += GetFrameTime();

    if (state->inventory.opened) return;

    if (showDialogue) { 
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
    
    if (inputMode && !startInput) HandleInputBox(state);
    if (gameTimer >= 60.0f) showMorseText = true;

    UpdateMorseFlash();
    UpdateComputerFlash();
}

void DrawPaperText(void)
{
    DrawRectangle(240, 250, 800, 200, Fade(LIGHTGRAY, 0.95f));
    DrawRectangleLines(240, 250, 800, 200, DARKGRAY);
    DrawText("After the sound disappears,\nonly light can speak.", 340, 330, 30, BLACK);
}

void DrawMorseTable(const GameState *state)
{
    int boxSize = 460; 
    int startX = (GetScreenWidth() - boxSize) / 2;
    int startY = (GetScreenHeight() - boxSize) / 2 - 40;

    Rectangle destRec = { (float)startX, (float)startY, (float)boxSize, (float)boxSize };
    Rectangle srcRec = { 0.0f, 0.0f, (float)state->codeSprite.width, (float)state->codeSprite.height };

    DrawTexturePro(state->codeSprite, srcRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);
    DrawRectangleLinesEx((Rectangle){destRec.x - 5, destRec.y - 5, destRec.width + 10, destRec.height + 10}, 4.0f, DARKGRAY);
}

void DrawNavigationCommand(void)
{
    DrawRectangle(240, 170, 800, 470, Fade(LIGHTGRAY, 0.95f));
    DrawRectangleLines(240, 170, 800, 470, DARKGRAY);
    DrawText("Navigation Command", 310, 220, 30, BLACK);
    DrawText("UUUU RRR D\nUU RR DDDD LLL D", 310, 310, 35, BLACK);
    DrawText("U R L D", 310, 500, 50, BLACK);
    DrawLineEx((Vector2){300, 550}, (Vector2){510, 550}, 5.0f, RED);
}

void DrawDialogue1(const GameState *state)
{
    DrawRectangle(150, 700, 980, 180, Fade(BLACK, 0.85f)); 
    DrawRectangleLines(150, 700, 980, 180, WHITE);

    if (dialogueLines != NULL && dialogueIndex < dialogueCount) {
        DrawTextEx(state->storyFont, dialogueLines[dialogueIndex], (Vector2){200, 730}, 32, 1, WHITE);
    }
    DrawText("[Press Z to Continue]", 850, 820, 20, GRAY);
}

void DrawLevel1(const GameState *state)
{
    // 1. 繪製信件紙張 letter.png
    if (!gotPaper) {
        Rectangle paperDest = { 530, 360, 150, 100 };
        Rectangle paperSrc = { 0.0f, 0.0f, (float)state->letterSprite.width, (float)state->letterSprite.height };
        DrawTexturePro(state->letterSprite, paperSrc, paperDest, (Vector2){0, 0}, 0.0f, WHITE);
    }

    // 💡 1. 閃爍的燈 (只有在沒有任何大視窗彈出時才繪製，保持視覺乾淨)
    if (morseLightOn) {
        DrawCircle(850, 205, 12, WHITE); 
    }

    // 2. 繪製摩斯書本 book.png
    if (!gotMorseTable) {
        Rectangle bookDest = { 850, 650, 200, 100 };
        Rectangle bookSrc = { 0.0f, 0.0f, (float)state->bookSprite.width, (float)state->bookSprite.height };
        DrawTexturePro(state->bookSprite, bookSrc, bookDest, (Vector2){0, 0}, 0.0f, WHITE);
    }

    // 3. 繪製控制台機器 device01.png
    Rectangle compDest = { 180, 350, 120, 140 };
    Rectangle compSrc = { 0.0f, 0.0f, (float)state->deviceSprite.width, (float)state->deviceSprite.height };
    DrawTexturePro(state->deviceSprite, compSrc, compDest, (Vector2){0, 0}, 0.0f, computerFlashColor); 
    
    // 4. 玩家繪製
    Rectangle playerSrc = { 0.0f, 0.0f, (float)state->playerSprite.width, (float)state->playerSprite.height };
    DrawTexturePro(state->playerSprite, playerSrc, player.rect, (Vector2){0, 0}, 0.0f, WHITE);

    // ---- 💡 摩斯密碼文字（長短線段）隱藏邏輯調整 ----
    // 加上額外判斷條件：!showPaperText && !showMorseTable && !showNavigationCommand
    if (showMorseText && !showPaperText && !showMorseTable && !showNavigationCommand) {
        DrawText(".._   ._.   ._..   _..", 400, 265, 30, WHITE); 
    }

    // ---- 介面與文字疊加層 ----
    if (showPaperText) {
        DrawPaperText();
    }
    if (showMorseTable) {
        DrawMorseTable(state); 
    }
    if (showNavigationCommand) {
        DrawNavigationCommand();
    }
    
    if (showDialogue){
        DrawDialogue1(state);
    }
    if (inputMode) {
        DrawRectangle(300, 800, 600, 80, Fade(WHITE, 0.9f));
        DrawRectangleLines(300, 800, 600, 80, DARKGRAY);
        DrawText(inputBuffer, 330, 820, 30, BLACK);
    }
}