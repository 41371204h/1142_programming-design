// TODO: add a rectangle that shows the player gets the key after entering the right answer

#include "raylib.h"
#include "game_shared.h"
#include "level2.h"
#include <string.h>
#include <stdbool.h>

// 💡 調整拼圖大小：將 TILE_SIZE 放大至 160，讓拼圖佔滿畫面的 8 成
#define PUZZLE_TILE_SIZE 160 
#define PLAYER_SIZE 70
#define PLAYER_SPEED 4

static Vector2 playerPos;

static Rectangle paperPile = {540, 350, 200, 200};
static Rectangle doorRect = {565, 800, 200, 200};

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
    if (IsKeyPressed(KEY_UP) && cursorY > 0)
        cursorY--;

    if (IsKeyPressed(KEY_DOWN) && cursorY < 2)
        cursorY++;

    if (IsKeyPressed(KEY_LEFT) && cursorX > 0)
        cursorX--;

    if (IsKeyPressed(KEY_RIGHT) && cursorX < 2)
        cursorX++;

    int currentIndex = cursorY * 3 + cursorX;

    if (IsKeyPressed(KEY_Z))
    {
        if (selectedTile == -1)
        {
            selectedTile = currentIndex;
        }
        else
        {
            if (selectedTile == currentIndex)
            {
                selectedTile = -1;
            }
            else
            {
                int temp = puzzle[selectedTile];
                puzzle[selectedTile] = puzzle[currentIndex];
                puzzle[currentIndex] = temp;

                selectedTile = -1;

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
    if (showDialogue)
    {
        AdvanceDialogue(state);
        return;
    }

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

    if (CheckCollisionRecs(playerRect, paperPile)){
        if (IsKeyPressed(KEY_Z)){
            showPuzzle = true;
            selectedTile = -1;
            StartDialogue(puzzleOpenDialogue, sizeof(puzzleOpenDialogue) / sizeof(puzzleOpenDialogue[0]), L2_DIALOGUE_NONE);
        }
    }
    if (CheckCollisionRecs(playerRect, doorRect)) {
        if (IsKeyPressed(KEY_Z)) {
            StartDialogue(doorLockedDialogue, sizeof(doorLockedDialogue) / sizeof(doorLockedDialogue[0]), L2_DIALOGUE_NONE);
            returnToHubAfterDialogue = false;
        }
    }
}

// ---------------------------
// 繪製拼圖 (置中、放大至 8 成、優化提示框與字體)
// ---------------------------
void DrawPuzzle(const GameState *state)
{
    // 💡 1. 繪製全螢幕的黑色底，達到完全覆蓋背景、跳出新視窗的強烈體感
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);

    // 💡 2. 計算拼圖總寬高與全置中起點
    int totalSize = 3 * PUZZLE_TILE_SIZE; // 3 * 160 = 480 像素
    int startX = (GetScreenWidth() - totalSize) / 2;
    int startY = (GetScreenHeight() - totalSize) / 2 - 40; // 稍微往上提給下方操作說明留空間

    // 💡 3. 繪製拼圖外框線
    DrawRectangleLinesEx((Rectangle){startX - 5, startY - 5, totalSize + 10, totalSize + 10}, 4.0f, DARKGRAY);

    for (int y = 0; y < 3; y++)
    {
        for (int x = 0; x < 3; x++)
        {
            int index = y * 3 + x;
            int tileValue = puzzle[index];

            Rectangle tileRect = {
                startX + x * PUZZLE_TILE_SIZE,
                startY + y * PUZZLE_TILE_SIZE,
                PUZZLE_TILE_SIZE,
                PUZZLE_TILE_SIZE
            };

            // 繪製對應的拼圖切片
            if (tileValue >= 1 && tileValue <= 9) {
                Rectangle srcRec = { 0.0f, 0.0f, (float)state->puzzleParts[tileValue - 1].width, (float)state->puzzleParts[tileValue - 1].height };
                DrawTexturePro(state->puzzleParts[tileValue - 1], srcRec, tileRect, (Vector2){0, 0}, 0.0f, WHITE);
            }

            // 💡 4. 優化外框提示顏色與粗細，避免黃框與背景沙子混色
            if (index == selectedTile)
            {
                // 已選取格：醒目的綠色內縮框
                DrawRectangleLinesEx(tileRect, 5.0f, GREEN);
            }

            if (x == cursorX && y == cursorY)
            {
                // 目前游標指著的格子：改用亮青色 (CYAN)，並疊加微微高亮透明色塊
                DrawRectangleLinesEx(tileRect, 5.0f, SKYBLUE);
                DrawRectangleRec(tileRect, Fade(WHITE, 0.15f));
            }
        }
    }

    // 💡 1. 將兩行文字拆開，定義各自的字串內容
    const char *line1 = "Select two puzzle pieces to swap their positions.";
    const char *line2 = "Arrow Keys: Move | Z: Select | X: Cancel";
    
    // 設定字型大小 (維持原樣或微調)
    int fontSize = 22; 

    // 💡 2. 使用 MeasureText 自動計算這兩行字在當前字型大小下的像素寬度
    int lineWidth1 = MeasureText(line1, fontSize);
    int lineWidth2 = MeasureText(line2, fontSize);

    // 💡 3. 精準計算出能讓文字水平置中的 X 座標
    int textX1 = (GetScreenWidth() - lineWidth1) / 2;
    int textX2 = (GetScreenWidth() - lineWidth2) / 2;

    // 💡 4. 繪製第一行字 (擺在拼圖正下方，startY + totalSize + 40 附近)
    DrawText(
        line1,
        textX1,
        startY + totalSize + 40,
        fontSize,
        LIGHTGRAY
    );

    // 💡 5. 繪製第二行字 (再往下移 30 像素)
    DrawText(
        line2,
        textX2,
        startY + totalSize + 40 + 30,
        fontSize,
        WHITE // 讓操作按鍵提示亮一點，視覺層次更好
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
// 繪製完成地圖 (精準對齊拼圖外框的正方形大小與位置)
// ---------------------------
void DrawCompletedMap(const GameState *state)
{
    // 💡 1. 這裡完全複製 DrawPuzzle 的大小與置中數學計算
    int totalSize = 3 * PUZZLE_TILE_SIZE; // 3 * 160 = 480 像素
    int startX = (GetScreenWidth() - totalSize) / 2;
    int startY = (GetScreenHeight() - totalSize) / 2 - 40; // 稍微往上提，與拼圖完全同位置

    // 💡 2. 將目標渲染範圍設定為與拼圖完全一樣的正方形
    Rectangle mapDestRec = { (float)startX, (float)startY, (float)totalSize, (float)totalSize };

    // 3. 獲取圖片原始大小範圍
    Rectangle mapSrcRec = { 0.0f, 0.0f, (float)state->mapSprite.width, (float)state->mapSprite.height };

    // 4. 渲染地圖圖片 (會自動完美縮放進 480x480 的正方形中)
    DrawTexturePro(state->mapSprite, mapSrcRec, mapDestRec, (Vector2){0, 0}, 0.0f, WHITE);

    // 5. 繪製外框線，粗細(4.0f)與顏色(DARKGRAY)皆與拼圖外框完全同步，達到無縫切換視覺效果
    DrawRectangleLinesEx((Rectangle){mapDestRec.x - 5, mapDestRec.y - 5, mapDestRec.width + 10, mapDestRec.height + 10}, 4.0f, DARKGRAY);
}

// ---------------------------
// 繪製 Level2
// ---------------------------
void DrawLevel2(const GameState *state)
{
    ClearBackground(DARKBLUE);

    // 💡 3. 控制隱藏機制：如果正在玩拼圖 (showPuzzle)，直接跳過繪製背景地圖所有物件與提示字
    if (!showPuzzle) 
    {
        // 渲染 paperPile.png 貼圖
        Rectangle paperSrcRec = { 0.0f, 0.0f, (float)state->paperPileSprite.width, (float)state->paperPileSprite.height };
        DrawTexturePro(state->paperPileSprite, paperSrcRec, paperPile, (Vector2){0, 0}, 0.0f, WHITE);

        // 💡 2. 將原本的褐色門改為完美渲染 door.png 貼圖
        Rectangle doorSrcRec = { 0.0f, 0.0f, (float)state->doorSprite.width, (float)state->doorSprite.height };
        DrawTexturePro(state->doorSprite, doorSrcRec, doorRect, (Vector2){0, 0}, 0.0f, WHITE); 

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

        // 左上角地圖提示文字
        DrawText(
            "Arrow Keys: Move | Z: Interact | C: Inventory",
            20,
            20,
            20,
            WHITE
        );
    }

    // 💡 如果點開了拼圖，則單獨跑獨立的大視窗渲染
    if (showPuzzle){
        DrawPuzzle(state); 
    }

    if (showMap){
        DrawCompletedMap(state); // 💡 修改：這裡要把 state 傳進去
    }

    if (showDialogue){
        DrawDialogue2(state);
    }
    
}