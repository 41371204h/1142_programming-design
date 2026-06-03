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
static bool showDialogue = false;

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
static const char *dialogueText =
"Looks like a map. This may be useful later.";

// ---------------------------
// 初始化拼圖
// ---------------------------
void InitPuzzle(void)
{
    int temp[9] = {
        2,1,3,
        4,5,6,
        7,8,9

        /* 正式版
        4,7,2,
        8,3,5,
        1,9,6
        */
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
                    showDialogue = true;

                    state -> isLevel2Cleared = true;

                    strcpy(state -> inventory.items[state -> inventory.count],
                    "Completed Map");

                    state -> inventory.count++;
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
        if (IsKeyPressed(KEY_Z))
        {
            showDialogue = false;

            state -> currentScreen = SCREEN_HUB;
        }

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
        }
    }
    // 與門互動
    if (CheckCollisionRecs(playerRect, doorRect)) {
        if (IsKeyPressed(KEY_Z)){
            state->currentScreen = SCREEN_HUB;
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
        "Arrow Keys: Move Cursor | Z: Select | X: Cancel",
        330,
        650,
        20,
        WHITE
    );
}

// ---------------------------
// 繪製對話框
// ---------------------------
void DrawDialogue2(void)
{
    DrawRectangle(150, 700, 980, 180, BLACK);

    DrawRectangleLines(150, 700, 980, 180, WHITE);

    DrawText(dialogueText, 200, 730, 30, WHITE);

    DrawText("[Press Z]", 900, 820, 20, GRAY);
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

    if (showDialogue){
        DrawDialogue2();
    }
}