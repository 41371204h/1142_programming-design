#include "raylib.h"
#include "level1.h"
#include "game_shared.h"
#include "audio.h"
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

// 💡 門被鎖住的對話內容
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

    // 看完對話後切換畫面的判斷
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
    Rectangle paperRect = {430, 360, 150, 100};      
    Rectangle morseRect = {850, 650, 200, 100};       
    Rectangle computerRect = {180, 350, 120, 140};  
    Rectangle doorRect = {1080, 375, 200, 200};     
    
    // 💡 物品欄碰撞設定
    Rectangle fakeKeyRect = { 80, 600, 50, 50 };
    Rectangle fakeComp1Rect = { 350, 655, 270, 180 }; 
    Rectangle fakeComp2Rect = { 630, 310, 150, 150 }; 

    if (CheckCollisionRecs(player.rect, fakeKeyRect)) {
        static const char *keyText[] = { "A simple key.", "This doesn't work for the electronic lock." };
        StartDialogue(keyText, 2, L1_DIALOGUE_START);
        return;
    }

    if (CheckCollisionRecs(player.rect, fakeComp1Rect)) {
        static const char *compText1[] = { "This terminal seems to have been shut down a long time ago.", "There is no reaction." };
        StartDialogue(compText1, 2, L1_DIALOGUE_START);
        return;
    }

    if (CheckCollisionRecs(player.rect, fakeComp2Rect)) {
        static const char *compText2[] = { "This is also a communication device.", "But it's completely dead." };
        StartDialogue(compText2, 2, L1_DIALOGUE_START);
        return;
    }

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
            returnToHubAfterDialogue = true; 
            
            state->isLevel1Cleared = true;
            strcpy(state->secretSequence, "UUUURRRDUURRDDDDLLLD");
            play_effect_win();
        } else {
            computerFlashTargetColor = Fade(RED, 0.6f); 
            computerFlashColor = computerFlashTargetColor;
            computerFlashCount = 3;
            computerFlashOn = true;
            flashTimer = 0;
            play_effect_fail();
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
    // 💡 核心優化：無論如何，時間軸與閃爍更新都不該被中斷
    gameTimer += GetFrameTime();
    UpdateMorseFlash();
    UpdateComputerFlash();

    // 💡 閃爍邏輯更新完後，再執行物品欄的阻斷
    if (state->inventory.opened) return;

    if (showDialogue) { 
        AdvanceDialogue(state);
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
        inputMode = false; 
    }
    
    if (inputMode && !startInput) HandleInputBox(state);
    if (gameTimer >= 60.0f) showMorseText = true;
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
    // 💡 落地陰影效果
    if (!gotPaper) {
        DrawEllipse(505, 455, 60, 15, Fade(BLACK, 0.40f)); 
        Rectangle paperDest = { 430, 360, 150, 100 };
        Rectangle paperSrc = { 0.0f, 0.0f, (float)state->letterSprite.width, (float)state->letterSprite.height };
        DrawTexturePro(state->letterSprite, paperSrc, paperDest, (Vector2){0, 0}, 0.0f, Fade(WHITE, 0.85f));
    }

    // 💡 摩斯訊號燈（還原乾淨圓點，且不受物品欄干擾、從頭到尾都會閃）
    if (morseLightOn) {
        DrawCircle(850, 205, 12, WHITE); 
    }

    if (!gotMorseTable) {
        DrawEllipse(950, 745, 80, 18, Fade(BLACK, 0.50f)); 
        Rectangle bookDest = { 850, 650, 200, 100 };
        Rectangle bookSrc = { 0.0f, 0.0f, (float)state->bookSprite.width, (float)state->bookSprite.height };
        DrawTexturePro(state->bookSprite, bookSrc, bookDest, (Vector2){0, 0}, 0.0f, Fade(WHITE, 0.88f));
    }

    // 💡 裝飾性鑰匙 (key.png) 繪製與落地陰影
    DrawEllipse(105, 640, 25, 8, Fade(BLACK, 0.35f)); 
    Rectangle keyDest = { 80, 600, 50, 50 };
    Rectangle keySrc = { 0.0f, 0.0f, (float)state->keySprite.width, (float)state->keySprite.height };
    DrawTexturePro(state->keySprite, keySrc, keyDest, (Vector2){0, 0}, 0.0f, Fade(GRAY, 0.55f)); 

    // 💡 誤導物 A (computer.png)
    DrawEllipse(365, 825, 55, 14, Fade(BLACK, 0.40f)); 
    Rectangle fakeComp1Dest = { 350, 655, 270, 180 }; 
    Rectangle fakeComp1Src = { 0.0f, 0.0f, (float)state->hubTerminalSprite.width, (float)state->hubTerminalSprite.height };
    DrawTexturePro(state->hubTerminalSprite, fakeComp1Src, fakeComp1Dest, (Vector2){0, 0}, 0.0f, Fade(GRAY, 0.65f));
    
    // 💡 誤導物 B：假電腦 (device02.png) 往左側平移
    DrawEllipse(705, 458, 55, 14, Fade(BLACK, 0.40f)); 
    Rectangle fakeComp2Dest = { 650, 330, 110, 130 };
    Rectangle compSrc2 = { 0.0f, 0.0f, (float)state->device02Sprite.width, (float)state->device02Sprite.height };
    DrawTexturePro(state->device02Sprite, compSrc2, fakeComp2Dest, (Vector2){0, 0}, 0.0f, Fade(GRAY, 0.55f));

    // 💡 真電腦控制台（解謎主螢幕）
    DrawEllipse(240, 488, 55, 14, Fade(BLACK, 0.45f)); 
    Rectangle compDest = { 180, 350, 120, 140 };
    Rectangle compSrc = { 0.0f, 0.0f, (float)state->deviceSprite.width, (float)state->deviceSprite.height };
    Color finalCompColor = ColorAlpha(computerFlashColor, 0.92f);
    DrawTexturePro(state->deviceSprite, compSrc, compDest, (Vector2){0, 0}, 0.0f, finalCompColor); 
    
    // 玩家人物繪製
    Rectangle playerSrc = { 0.0f, 0.0f, (float)state->playerSprite.width, (float)state->playerSprite.height };
    DrawTexturePro(state->playerSprite, playerSrc, player.rect, (Vector2){0, 0}, 0.0f, WHITE);

    if (showMorseText && !showPaperText && !showMorseTable && !showNavigationCommand) {
        DrawText(".._   ._.   ._..   _..", 400, 265, 30, WHITE); 
    }

    // 懸浮 UI 視窗渲染
    if (showPaperText) {
        DrawPaperText();
    }
    if (showMorseTable) {
        DrawMorseTable(state); 
    }
    if (showNavigationCommand) {
        DrawNavigationCommand();
    }
    
    // 當有劇情對話框時，強制不畫下方的輸入文字框，避免雜亂重疊
    if (showDialogue){
        DrawDialogue1(state);
    } else if (inputMode) {
        DrawRectangle(300, 800, 600, 80, Fade(WHITE, 0.9f));
        DrawRectangleLines(300, 800, 600, 80, DARKGRAY);
        DrawText(inputBuffer, 330, 820, 30, BLACK);
    }
}