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
// 💡 將初始閃爍顏色改為全透明 WHITE，這樣平常機械主機才不會變暗
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
    returnToHubAfterDialogue = false;
    StartDialogue(initialDialogue, sizeof(initialDialogue) / sizeof(initialDialogue[0]), L1_DIALOGUE_START);

    inputMode = false;
    gameTimer = 0;
    morseIndex = 0;
    morseTimer = 0.0f;
    morseLightOn = true;

    showMorseText = false;
    computerFlashColor = WHITE; // 預設正常不發光濾鏡
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
}

static void HandleInteraction(GameState *state)
{
    // 💡 調整判定盒大小，與新圖片尺寸完全對齊
    Rectangle paperRect = {580, 380, 80, 80};      // letter 放大到 80x80
    Rectangle morseRect = {850, 600, 100, 120};     // book.png 100x120
    Rectangle computerRect = {100, 350, 100, 120};  // device01.png 100x120
    Rectangle doorRect = {1150, 375, 100, 200};     // door.png 100x200 寬高

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
            // 💡 密碼正確：開啟綠色濾鏡閃爍
            computerFlashTargetColor = Fade(GREEN, 0.6f); 
            computerFlashColor = computerFlashTargetColor;
            computerFlashCount = 3; // 閃三次
            computerFlashOn = true;
            flashTimer = 0;

            if (!gotNavigationCode) {
                AddItem(state, "Navigation Command");
                gotNavigationCode = true;
            }
            showNavigationCommand = true;
            StartDialogue(successDialogue, sizeof(successDialogue) / sizeof(successDialogue[0]), L1_DIALOGUE_SUCCESS);
            returnToHubAfterDialogue = true;
            
            state->isLevel1Cleared = true;
            strcpy(state->secretSequence, "URLD");
        } else {
            // 💡 密碼錯誤：開啟紅色濾鏡閃爍
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
            computerFlashColor = WHITE; // 閃爍暗掉時，恢復原圖真實色彩 (WHITE)
            computerFlashOn = false;
            computerFlashCount--;
        } else if (computerFlashCount > 0) {
            computerFlashColor = computerFlashTargetColor; // 亮起對應顏色
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
    DrawRectangle(240, 250, 800, 200, LIGHTGRAY);
    DrawText("After the sound disappears,\nonly light can speak.", 340, 330, 30, BLACK);
}

// 💡 點開摩斯表後，完美渲染正方形的 code.png
void DrawMorseTable(const GameState *state)
{
    int boxSize = 460; // 正方形大小
    int startX = (GetScreenWidth() - boxSize) / 2;
    int startY = (GetScreenHeight() - boxSize) / 2 - 40;

    Rectangle destRec = { (float)startX, (float)startY, (float)boxSize, (float)boxSize };
    Rectangle srcRec = { 0.0f, 0.0f, (float)state->codeSprite.width, (float)state->codeSprite.height };

    DrawTexturePro(state->codeSprite, srcRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);
    DrawRectangleLinesEx((Rectangle){destRec.x - 5, destRec.y - 5, destRec.width + 10, destRec.height + 10}, 4.0f, DARKGRAY);
}

void DrawNavigationCommand(void)
{
    DrawRectangle(240, 170, 800, 470, LIGHTGRAY);
    DrawText("Navigation Command", 310, 220, 30, BLACK);
    DrawText("UUUU RRR D\nUU RR DDDD LLL D", 310, 310, 35, BLACK);
    DrawText("U R L D", 310, 500, 50, BLACK);
    DrawLineEx((Vector2){300, 550}, (Vector2){510, 550}, 5.0f, RED);
}

void DrawDialogue1(const GameState *state)
{
    DrawRectangle(150, 700, 980, 180, BLACK);
    DrawRectangleLines(150, 700, 980, 180, WHITE);

    if (dialogueLines != NULL && dialogueIndex < dialogueCount) {
        DrawTextEx(state->storyFont, dialogueLines[dialogueIndex], (Vector2){200, 730}, 32, 1, WHITE);
    }
    DrawText("[Press Z to Continue]", 850, 820, 20, GRAY);
}

void DrawLevel1(const GameState *state)
{
    // 💡 1. 繪製門 (由 doorRect 放大至 100x200)
    Rectangle doorDest = { 1080, 375, 200, 200 };
    Rectangle doorSrc = { 0.0f, 0.0f, (float)state->doorSprite.width, (float)state->doorSprite.height };
    DrawTexturePro(state->doorSprite, doorSrc, doorDest, (Vector2){0, 0}, 0.0f, WHITE);

    // 💡 2. 繪製中間放大的信件紙張 letter.png (原本 40x40 放大至 80x80)
    if (!gotPaper) {
        Rectangle paperDest = { 580, 380, 150, 100 };
        Rectangle paperSrc = { 0.0f, 0.0f, (float)state->letterSprite.width, (float)state->letterSprite.height };
        DrawTexturePro(state->letterSprite, paperSrc, paperDest, (Vector2){0, 0}, 0.0f, WHITE);
    }

    // 摩斯光點
    if (morseLightOn) DrawCircle(640, 700, 20, WHITE); 

    // 💡 3. 繪製右下角摩斯書本 book.png (100x120)
    if (!gotMorseTable) {
        Rectangle bookDest = { 850, 600, 200, 90 };
        Rectangle bookSrc = { 0.0f, 0.0f, (float)state->bookSprite.width, (float)state->bookSprite.height };
        DrawTexturePro(state->bookSprite, bookSrc, bookDest, (Vector2){0, 0}, 0.0f, WHITE);
    }

    // 💡 4. 繪製通訊控制台機器 device01.png 並綁定紅綠發光濾鏡 (Tint)
    Rectangle compDest = { 100, 350, 120, 140 };
    Rectangle compSrc = { 0.0f, 0.0f, (float)state->deviceSprite.width, (float)state->deviceSprite.height };
    // 關鍵所在：將原本純白改成 computerFlashColor，達到半透明顏色閃爍效果！
    DrawTexturePro(state->deviceSprite, compSrc, compDest, (Vector2){0, 0}, 0.0f, computerFlashColor); 
    
    // 玩家繪製
    Rectangle playerSrc = { 0.0f, 0.0f, (float)state->playerSprite.width, (float)state->playerSprite.height };
    DrawTexturePro(state->playerSprite, playerSrc, player.rect, (Vector2){0, 0}, 0.0f, WHITE);

    if (showPaperText) {
        DrawPaperText();
    }
    if (showMorseTable) {
        DrawMorseTable(state); // 💡 傳入 state 繪製 code.png
    }
    if (showNavigationCommand) {
        DrawNavigationCommand();
    }
    if (showMorseText) {
        DrawText(".._   ._.   ._..   _..", 350, 50, 30, WHITE); 
    }
    if (showDialogue){
        DrawDialogue1(state);
    }
    if (inputMode) {
        DrawRectangle(300, 800, 600, 80, WHITE);
        DrawText(inputBuffer, 330, 820, 30, BLACK);
    }
}