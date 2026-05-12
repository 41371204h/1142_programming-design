#include "raylib.h"
#include <string.h>
#include <stdbool.h>

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 960

#define PLAYER_SIZE 40
#define PLAYER_SPEED 4

#define MAX_ITEMS 10

// =============================
// 場景狀態
// =============================
typedef enum {
    SCENE_MENU,
    SCENE_LEVEL1
} Scene;

// =============================
// 玩家結構
// =============================
typedef struct {
    Rectangle rect;
} Player;

// =============================
// 物品欄結構
// =============================
typedef struct {
    char items[MAX_ITEMS][100];
    int count;
    bool opened;
    int selected;
} Inventory;

// =============================
// 全域變數
// =============================
Scene currentScene = SCENE_MENU;

Player player;
Inventory inventory;

// 是否已取得物品
bool gotPaper = false;
bool gotMorseTable = false;
bool gotNavigationCode = false;

// 是否完成第一關
bool level1Completed = false;

// 顯示文字訊息
bool showPaperText = false;
bool showMorseTable = false;

// 電腦輸入模式
bool inputMode = false;
char inputBuffer[20] = "";
int inputLength = 0;

// 電腦閃燈狀態
int computerFlashCount = 0;
float flashTimer = 0;
Color computerFlashColor = GRAY;

// 計時器
float gameTimer = 0;

// 摩斯顯示
bool showMorseText = false;

// 主畫面顯示文字
char menuText[100] = "main menu\npress Z to start";

// =============================
// 加入物品欄
// =============================
void AddItem(const char *itemName)
{
    if (inventory.count < MAX_ITEMS)
    {
        strcpy(inventory.items[inventory.count], itemName);
        inventory.count++;
    }
}

// =============================
// 初始化遊戲
// =============================
void InitGame()
{
    player.rect = (Rectangle){1100, 450, PLAYER_SIZE, PLAYER_SIZE};

    inventory.count = 0;
    inventory.opened = false;
    inventory.selected = 0;
}

// =============================
// 更新玩家移動
// =============================
void UpdatePlayer()
{
    if (IsKeyDown(KEY_UP))
        player.rect.y -= PLAYER_SPEED;

    if (IsKeyDown(KEY_DOWN))
        player.rect.y += PLAYER_SPEED;

    if (IsKeyDown(KEY_LEFT))
        player.rect.x -= PLAYER_SPEED;

    if (IsKeyDown(KEY_RIGHT))
        player.rect.x += PLAYER_SPEED;
}

// =============================
// 畫主畫面
// =============================
void DrawMenu()
{
    ClearBackground(BLACK);

    DrawText(menuText, 400, 450, 40, WHITE);
}

// =============================
// 畫物品欄
// =============================
void DrawInventory()
{
    DrawRectangle(200, 700, 880, 200, DARKGRAY);

    for (int i = 0; i < inventory.count; i++)
    {
        Color color = (i == inventory.selected) ? YELLOW : WHITE;
        DrawText(inventory.items[i], 250, 740 + i * 30, 25, color);
    }
}

// =============================
// 畫第一關場景
// =============================
void DrawLevel1()
{
    ClearBackground(BLACK);

    // 門（右方）
    DrawRectangle(1150, 400, 80, 150, BROWN);

    // 紙（中間）
    if (!gotPaper)
        DrawRectangle(600, 400, 40, 40, WHITE);

    // 摩斯閃爍點（中下）
    bool blink = ((int)(GetTime() * 4) % 2 == 0);
    if (blink)
        DrawCircle(640, 700, 20, WHITE);

    // 摩斯表（素材預留）
    if (!gotMorseTable)
        DrawRectangle(850, 600, 100, 120, BLUE);

    // 電腦（左方）
    DrawRectangle(100, 350, 100, 120, computerFlashColor);

    // 玩家
    DrawRectangleRec(player.rect, RED);

    // 紙內容
    if (showPaperText)
    {
        DrawRectangle(250, 250, 800, 200, LIGHTGRAY);
        DrawText("After the sound disappears,\n only light can speak", 350, 330, 30, BLACK);
    }

    // 摩斯表內容
    if (showMorseTable)
    {
        DrawRectangle(850, 150, 300, 400, LIGHTGRAY);
        DrawText("Morse Code Table\n - Placeholder", 880, 300, 25, BLACK);
    }

    // 60秒後顯示文字提示
    if (showMorseText)
    {
        DrawText(".._   ._.   _..   ._..", 350, 50, 30, WHITE);
    }

    // 輸入框
    if (inputMode)
    {
        DrawRectangle(300, 800, 600, 80, WHITE);
        DrawText(inputBuffer, 330, 820, 30, BLACK);
    }

    // 物品欄
    if (inventory.opened)
    {
        DrawInventory();
    }
}

// =============================
// 處理互動
// =============================
void HandleInteraction()
{
    Rectangle paperRect = {600, 400, 40, 40};
    Rectangle morseRect = {850, 600, 100, 120};
    Rectangle computerRect = {100, 350, 100, 120};
    Rectangle doorRect = {1150, 400, 80, 150};

    // 紙互動
    if (CheckCollisionRecs(player.rect, paperRect) && !gotPaper)
    {
        showPaperText = true;

        if (!gotPaper)
        {
            AddItem("paper with clue");
            gotPaper = true;
        }
    }

    // 摩斯表互動
    if (CheckCollisionRecs(player.rect, morseRect) && !gotMorseTable)
    {
        showMorseTable = true;

        if (!gotMorseTable)
        {
            AddItem("Morse Code Table");
            gotMorseTable = true;
        }
    }

    // 電腦互動
    if (CheckCollisionRecs(player.rect, computerRect))
    {
        inputMode = true;
        inputLength = 0;
        memset(inputBuffer, 0, sizeof(inputBuffer));
    }

    // 門互動
    if (CheckCollisionRecs(player.rect, doorRect))
    {
        currentScene = SCENE_MENU;

        if (level1Completed)
            strcpy(menuText, "Main Menu\n Level 1 Completed");
        else
            strcpy(menuText, "Main Menu\n Press Z to Continue");
    }
}

// =============================
// 處理輸入框
// =============================
void HandleInputBox()
{
    int key = GetCharPressed();

    while (key > 0)
    {
        if ((key >= 32) && (key <= 125) && (inputLength < 10))
        {
            inputBuffer[inputLength] = (char)key;
            inputLength++;
        }

        key = GetCharPressed();
    }

    // Backspace
    if (IsKeyPressed(KEY_BACKSPACE) && inputLength > 0)
    {
        inputLength--;
        inputBuffer[inputLength] = '\0';
    }

    // Enter送出答案
    if (IsKeyPressed(KEY_ENTER))
    {
        if (strcmp(inputBuffer, "URDL") == 0)
        {
            computerFlashColor = GREEN;
            computerFlashCount = 2;

            if (!gotNavigationCode)
            {
                AddItem("Navigation Command: U R D L");
                gotNavigationCode = true;
            }

            level1Completed = true;
        }
        else
        {
            computerFlashColor = RED;
            computerFlashCount = 2;
        }

        inputMode = false;
    }
}

// =============================
// 電腦閃爍效果
// =============================
void UpdateComputerFlash()
{
    if (computerFlashCount > 0)
    {
        flashTimer += GetFrameTime();

        if (flashTimer >= 0.3f)
        {
            flashTimer = 0;
            computerFlashCount--;

            if (computerFlashCount == 0)
            {
                computerFlashColor = GRAY;
            }
        }
    }
}

// =============================
// 主程式
// =============================
int main()
{
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Level 1 Prototype");

    SetTargetFPS(60);

    InitGame();

    while (!WindowShouldClose())
    {
        // =============================
        // 更新
        // =============================
        if (currentScene == SCENE_MENU)
        {
            if (IsKeyPressed(KEY_Z))
            {
                currentScene = SCENE_LEVEL1;
            }
        }
        else if (currentScene == SCENE_LEVEL1)
        {
            gameTimer += GetFrameTime();

            UpdatePlayer();

            // Z互動
            if (IsKeyPressed(KEY_Z))
            {
                HandleInteraction();
            }

            // X取消查看
            if (IsKeyPressed(KEY_X))
            {
                showPaperText = false;
                showMorseTable = false;
            }

            // C打開物品欄
            if (IsKeyPressed(KEY_C))
            {
                inventory.opened = !inventory.opened;
            }

            // 物品欄選擇
            if (inventory.opened)
            {
                if (IsKeyPressed(KEY_UP))
                {
                    if (inventory.selected > 0)
                        inventory.selected--;
                }

                if (IsKeyPressed(KEY_DOWN))
                {
                    if (inventory.selected < inventory.count - 1)
                        inventory.selected++;
                }
            }

            // 電腦輸入
            if (inputMode)
            {
                HandleInputBox();
            }

            // 60秒後出現摩斯提示
            if (gameTimer >= 60.0f)
            {
                showMorseText = true;
            }

            UpdateComputerFlash();
        }

        // =============================
        // 畫面
        // =============================
        BeginDrawing();

        if (currentScene == SCENE_MENU)
        {
            DrawMenu();
        }
        else if (currentScene == SCENE_LEVEL1)
        {
            DrawLevel1();
        }

        EndDrawing();
    }

    CloseWindow();

    return 0;
}