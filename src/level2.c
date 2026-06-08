#include "raylib.h"
#include "game_shared.h"
#include "level2.h"
#include <string.h>
#include <stdbool.h>


#define TILE_SIZE 80

#define PLAYER_SIZE 70
#define PLAYER_SPEED 4

static Vector2 playerPos;

static Rectangle paperPile = {540, 350, 200, 200};
static Rectangle doorRect = {565, 850, 150, 80};

// ---------------------------
// UI 狀態
// ---------------------------
static bool showPuzzle = false;
static bool showMap = false;
static bool showDialogue = false;
static bool returnToHubAfterDialogue = false;

// ---------------------------
// 拼圖
// ---------------------------
static int puzzle[9];
static int selectedTile = -1;
static int cursorX = 0;
static int cursorY = 0;

// ---------------------------
// 對話
// ---------------------------
typedef enum {
    L2_DIALOGUE_NONE,
    L2_DIALOGUE_COMPLETE
} L2DialogueMode;

static const char **dialogueLines = NULL;
static int dialogueCount = 0;
static int dialogueIndex = 0;
static L2DialogueMode dialogueMode = L2_DIALOGUE_NONE;

static const char *enterDialogue[] = {
    "Strange...\nI can't remember the internal structure of\nthe escape zone.",
    "This won't do,\nI'll have to go to the archives to look for it."
};

static const char *puzzleOpenDialogue[] = {
    "These papers are all damaged...",
    "I need to fix it,\nbut how could I know which piece goes where?"
};

static const char *levelCompleteDialogue[] = {
    "That's it!\nIt must be the map of the escape zone!",
    "I need to hurry up before the air runs out."
};

static const char *doorLockedDialogue[] = {
    "I need to find the map first."
};

static void StartDialogue(const char **lines, int count, L2DialogueMode mode)
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

    if (dialogueMode == L2_DIALOGUE_COMPLETE) {
        showMap = false;
        state->currentScreen = SCREEN_HUB;
    }

    if (returnToHubAfterDialogue) {
        returnToHubAfterDialogue = false;
        state->currentScreen = SCREEN_HUB;
    }

    dialogueMode = L2_DIALOGUE_NONE;
}

static void AdvanceDialogue(GameState *state)
{
    if (!IsKeyPressed(KEY_Z)) return;

    dialogueIndex++;
    if (dialogueIndex >= dialogueCount) {
        FinishDialogue(state);
    }
}

// ---------------------------
// 初始化拼圖
// ---------------------------
void InitPuzzle(void)
{
    int temp[9] = {
        /* 測試版
        2,1,3,
        4,5,6,
        7,8,9
        */

        // /* 正式版
        4,7,2,
        8,3,5,
        1,9,6
        // */
    };

    for (int i = 0; i < 9; i++)
    {
        puzzle[i] = temp[i];
    }
}

// ---------------------------
// 判斷拼圖完成
// ---------------------------
bool IsPuzzleSolved(void)
{
    for (int i = 0; i < 9; i++)
    {
        if (puzzle[i] != i + 1)
            return false;
    }

    return true;
}

// ---------------------------
// 初始化 Level2
// ---------------------------
void InitLevel2(void)
{
    playerPos = (Vector2){600, 800};
    showPuzzle = false;
    showMap = false;
    showDialogue = false;
    returnToHubAfterDialogue = false;
    selectedTile = -1;
    cursorX = 0;
    cursorY = 0;
    StartDialogue(enterDialogue, sizeof(enterDialogue) / sizeof(enterDialogue[0]), L2_DIALOGUE_NONE);

    InitPuzzle();
}

// ---------------------------
// 玩家移動
// ---------------------------
void UpdatePlayer(void)
{
    if (IsKeyDown(KEY_UP))
        playerPos.y -= PLAYER_SPEED;

    if (IsKeyDown(KEY_DOWN))
        playerPos.y += PLAYER_SPEED;

    if (IsKeyDown(KEY_LEFT))
        playerPos.x -= PLAYER_SPEED;

    if (IsKeyDown(KEY_RIGHT))
        playerPos.x += PLAYER_SPEED;
}

// ---------------------------
// 更新拼圖
// ---------------------------
void UpdatePuzzle(GameState *state)
{
    // 移動游標
    if (IsKeyPressed(KEY_UP) && cursorY > 0)
        cursorY--;

    if (IsKeyPressed(KEY_DOWN) && cursorY < 2)
        cursorY++;

    if (IsKeyPressed(KEY_LEFT) && cursorX > 0)
        cursorX--;

    if (IsKeyPressed(KEY_RIGHT) && cursorX < 2)
        cursorX++;

    int currentIndex = cursorY * 3 + cursorX;

    // 選擇
    if (IsKeyPressed(KEY_Z))
    {
        if (selectedTile == -1)
        {
            selectedTile = currentIndex;
        }
        else
        {
            // 同一格取消
            if (selectedTile == currentIndex)
            {
                selectedTile = -1;
            }
            else
            {
                // 交換
                int temp = puzzle[selectedTile];
                puzzle[selectedTile] = puzzle[currentIndex];
                puzzle[currentIndex] = temp;

                selectedTile = -1;

                // 判斷完成
                if (IsPuzzleSolved())
                {
                    showPuzzle = false;
                    showMap = true;
                    StartDialogue(levelCompleteDialogue, sizeof(levelCompleteDialogue) / sizeof(levelCompleteDialogue[0]), L2_DIALOGUE_COMPLETE);

                    state -> isLevel2Cleared = true;

                    AddItem(state, "Completed Map");
                }
            }
        }
    }

    // 取消
    if (IsKeyPressed(KEY_X))
    {
        selectedTile = -1;
    }
}

// ---------------------------
// 更新 Level2
// ---------------------------
void UpdateLevel2(GameState *state)
{
    if (state->inventory.opened) return;
    // 對話框
    if (showDialogue)
    {
        AdvanceDialogue(state);

        return;
    }

    // 拼圖模式
    if (showPuzzle)
    {
        UpdatePuzzle(state);
        return;
    }

    UpdatePlayer();

    Rectangle playerRect = {
        playerPos.x,
        playerPos.y,
        PLAYER_SIZE,
        PLAYER_SIZE
    };

    // 與紙堆互動
    if (CheckCollisionRecs(playerRect, paperPile)){
        if (IsKeyPressed(KEY_Z)){
            showPuzzle = true;
            selectedTile = -1;
            StartDialogue(puzzleOpenDialogue, sizeof(puzzleOpenDialogue) / sizeof(puzzleOpenDialogue[0]), L2_DIALOGUE_NONE);
        }
    }
    // 與門互動
    if (CheckCollisionRecs(playerRect, doorRect)) {
        if (IsKeyPressed(KEY_Z)){
            StartDialogue(doorLockedDialogue, sizeof(doorLockedDialogue) / sizeof(doorLockedDialogue[0]), L2_DIALOGUE_NONE);
            returnToHubAfterDialogue = false;
        }
    }
}

// ---------------------------
// 繪製拼圖
// ---------------------------
void DrawPuzzle(void)
{
    DrawRectangle(250, 120, 780, 720, BLACK);

    for (int y = 0; y < 3; y++)
    {
        for (int x = 0; x < 3; x++)
        {
            int index = y * 3 + x;

            Rectangle tileRect = {
                400 + x * TILE_SIZE,
                220 + y * TILE_SIZE,
                TILE_SIZE,
                TILE_SIZE
            };

            Color color = GRAY;

            if (index == selectedTile)
                color = GREEN;

            if (x == cursorX && y == cursorY)
                color = YELLOW;

            DrawRectangleRec(tileRect, color);

            DrawText(
                TextFormat("%d", puzzle[index]),
                tileRect.x + 30,
                tileRect.y + 25,
                30,
                BLACK
            );
        }
    }

    DrawText(
        "Select two puzzle pieces to swap their positions.\nArrow Keys: Move | Z: Select | X: Cancel",
        330,
        650,
        20,
        WHITE
    );
}

// ---------------------------
// 繪製對話框
// ---------------------------
void DrawDialogue2(const GameState *state)
{
    DrawRectangle(150, 700, 980, 180, BLACK);

    DrawRectangleLines(150, 700, 980, 180, WHITE);

    if (dialogueLines != NULL && dialogueIndex < dialogueCount) {
        DrawTextEx(state->storyFont, dialogueLines[dialogueIndex], (Vector2){200, 730}, 32, 1, WHITE);
    }

    DrawText("[Press Z to Continue]", 850, 820, 20, GRAY);
}

// ---------------------------
// 繪製完成地圖
// ---------------------------
void DrawCompletedMap(void)
{
    DrawRectangle(260, 150, 760, 460, LIGHTGRAY);
    DrawRectangleLines(260, 150, 760, 460, DARKGRAY);
    DrawText("Completed Map", 330, 210, 35, BLACK); // 標題可以去掉
    DrawText("Map placeholder", 330, 290, 24, BLACK);
}

// ---------------------------
// 繪製 Level2
// ---------------------------
void DrawLevel2(const GameState *state)
{
    ClearBackground(DARKBLUE);

    DrawRectangleRec(paperPile, BROWN); // 紙堆
    DrawRectangleRec(doorRect, BROWN); // 門

    DrawText(
        "Pile of papers",
        paperPile.x + 20,
        paperPile.y + 80,
        20,
        WHITE
    );

    // 玩家
    Rectangle sourceRec = {
        0.0f,
        0.0f,
        (float)state->playerSprite.width,
        (float)state->playerSprite.height
    };
    Rectangle destRec = {
        playerPos.x,
        playerPos.y,
        PLAYER_SIZE,
        PLAYER_SIZE
    };
    DrawTexturePro(state->playerSprite, sourceRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);

    DrawText(
        "Arrow Keys: Move | Z: Interact | C: Inventory",
        20,
        20,
        20,
        WHITE
    );

    if (showPuzzle){
        DrawPuzzle();
    }

    if (showMap){
        DrawCompletedMap();
    }

    if (showDialogue){
        DrawDialogue2(state);
    }
}
