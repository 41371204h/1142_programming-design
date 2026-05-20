#include "raylib.h"
#include "game_shared.h"
#include "level3.h"
#include <string.h>

// --- 關卡設定 ---
#define TILE_SIZE 70
#define MAZE_COLS 8
#define MAZE_ROWS 8

// 定義第三關的內部狀態
typedef enum {
    L3_STORY,   // 顯示故事開場白
    L3_PLAYING, // 實際遊玩與計時中
    L3_FAILED   // 失敗黑屏
} L3State;

// 迷宮初始模板 (每次重置都會拷貝這份)
// 1=wall, 0=path, 2=tool(North East), 3=Core device(center), 4=starting point
const int defaultMaze[MAZE_ROWS][MAZE_COLS] = {
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 0, 0, 0, 0, 0, 2, 1},
    {1, 0, 0, 0, 0, 0, 0, 1},
    {1, 0, 0, 1, 0, 1, 1, 1},
    {1, 0, 0, 1, 3, 0, 0, 1},
    {1, 0, 0, 1, 1, 1, 0, 1},
    {1, 4, 0, 0, 0, 0, 0, 1},
    {1, 1, 1, 1, 1, 1, 1, 1}
};

// --- 關卡狀態變數 ---
static L3State currentState = L3_STORY;
static int maze[MAZE_ROWS][MAZE_COLS];
static int playerX = 1;      
static int playerY = 6;      // 讓玩家出生在左下角
static float timeLeft = 5.0f; ///
static bool hasHandle = false; 
static bool showLockUI = false; 
static char inputBuffer[5] = ""; 
static int inputIndex = 0;

// 👇 新增：紀錄在失敗畫面停留了多久
static float failedTimer = 0.0f; 

// --- 重置關卡的專屬函數 ---
static void ResetLevel3(void) {
    currentState = L3_STORY; 
    playerX = 1;
    playerY = 6;
    timeLeft = 5.0f; ///
    hasHandle = false;
    showLockUI = false;
    inputIndex = 0;
    memset(inputBuffer, 0, sizeof(inputBuffer));
    
    // 👇 新增：每次重置關卡時，也要把失敗計時器歸零
    failedTimer = 0.0f; 
    
    for(int y = 0; y < MAZE_ROWS; y++) {
        for(int x = 0; x < MAZE_COLS; x++) {
            maze[y][x] = defaultMaze[y][x];
        }
    }
}

void UpdateLevel3(GameState *state) {
    // 第一次進入關卡時，確保地圖已經載入
    static bool isInitialized = false;
    if (!isInitialized) {
        ResetLevel3();
        isInitialized = true;
    }

    switch (currentState) {
        case L3_STORY:
            // 等待玩家按下空白鍵開始遊戲
            if (IsKeyPressed(KEY_SPACE)) {
                currentState = L3_PLAYING;
            }
            break;

        case L3_FAILED:
            // 1. 持續累加失敗畫面的停留時間
            failedTimer += GetFrameTime(); 

            // 2. 經過 10 秒後，才進入「第二階段」並允許玩家按鍵
            if (failedTimer > 10.0f) {
                if (IsKeyPressed(KEY_SPACE)) {
                    ResetLevel3();
                } else if (IsKeyPressed(KEY_ESCAPE)) {
                    ResetLevel3();
                    state->currentScreen = SCREEN_HUB; 
                }
            }
            break;

        case L3_PLAYING:
            timeLeft -= GetFrameTime();
            if (timeLeft <= 0) {
                timeLeft = 0;
                currentState = L3_FAILED; 
                failedTimer = 0.0f; // 👇 確保剛進入失敗狀態時，計時器是 0
                break;
            }

            // 2. 密碼輸入邏輯
            if (showLockUI) {
                if (IsKeyPressed(KEY_UP) && inputIndex < 4) inputBuffer[inputIndex++] = 'U';
                if (IsKeyPressed(KEY_DOWN) && inputIndex < 4) inputBuffer[inputIndex++] = 'D';
                if (IsKeyPressed(KEY_LEFT) && inputIndex < 4) inputBuffer[inputIndex++] = 'L';
                if (IsKeyPressed(KEY_RIGHT) && inputIndex < 4) inputBuffer[inputIndex++] = 'R';

                if (inputIndex == 4) {
                    inputBuffer[4] = '\0';
                    if (strcmp(inputBuffer, state->secretSequence) == 0) {
                        // 密碼正確！過關
                        state->isLevel3Cleared = true;
                        state->currentScreen = SCREEN_HUB; // 回到主畫面
                        ResetLevel3(); // 為下一次遊玩重置
                    } else {
                        // 密碼錯誤，清空重打 (但不中斷遊戲與時間)
                        inputIndex = 0;
                        memset(inputBuffer, 0, sizeof(inputBuffer));
                    }
                }
                
                if (IsKeyPressed(KEY_ESCAPE)) showLockUI = false;
                break; // 輸入密碼時不可移動
            }

            // 3. 玩家移動邏輯
            int nextX = playerX;
            int nextY = playerY;

            if (IsKeyPressed(KEY_RIGHT)) nextX++;
            if (IsKeyPressed(KEY_LEFT))  nextX--;
            if (IsKeyPressed(KEY_UP))    nextY--;
            if (IsKeyPressed(KEY_DOWN))  nextY++;

            // 碰撞判定
            if (nextX >= 0 && nextX < MAZE_COLS && nextY >= 0 && nextY < MAZE_ROWS) {
                if (maze[nextY][nextX] != 1) { 
                    playerX = nextX;
                    playerY = nextY;

                    // 檢查拉柄 (2)
                    if (maze[playerY][playerX] == 2) {
                        hasHandle = true;
                        maze[playerY][playerX] = 0; // 拿走後變空地
                    }
                    
                    // 檢查核心裝置 (3)
                    if (maze[playerY][playerX] == 3 && hasHandle) {
                        showLockUI = true;
                    }
                }
            }
            break;
    }
}
void DrawLevel3(void) {
    // 如果是失敗狀態，畫出兩階段的黑屏
    if (currentState == L3_FAILED) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);
        
        if (failedTimer <= 10.0f) {
            // 第一階段 (0~10秒)：只顯示故事文字，放在畫面正中央
            DrawText("You hear a familiar voice saying", 
                     GetScreenWidth()/2 - 580, GetScreenHeight()/2 - 280, 35, GRAY);
            DrawText("Wake up! You're almost there.\nDon't fall asleep...",
                     GetScreenWidth()/2 - 580, GetScreenHeight()/2 - 80, 50, WHITE);
        } else {
            // 第二階段 (10秒後)：顯示系統關閉與按鍵指示
            DrawText("SYSTEM SHUTDOWN", GetScreenWidth()/2 - 380, 350, 80, RED);
            DrawText("[ Press SPACE to Restart Level 3 ]", GetScreenWidth()/2 - 250, 550, 30, WHITE);
            DrawText("[ Press ESC to Return to Main Hub ]", GetScreenWidth()/2 - 230, 610, 25, DARKGRAY);
        }
        
        return; // 畫完失敗畫面就結束，不畫底下的迷宮
    }

    // --- 繪製迷宮與遊戲本體 ---
    int offsetX = (GetScreenWidth() - (MAZE_COLS * TILE_SIZE)) / 2; // 置中顯示
    int offsetY = 180;

    for (int y = 0; y < MAZE_ROWS; y++) {
        for (int x = 0; x < MAZE_COLS; x++) {
            int drawX = offsetX + x * TILE_SIZE;
            int drawY = offsetY + y * TILE_SIZE;

            if (maze[y][x] == 1) { // wall
                DrawRectangle(drawX, drawY, TILE_SIZE, TILE_SIZE, BLACK);
                DrawRectangleLines(drawX, drawY, TILE_SIZE, TILE_SIZE, WHITE);
            } else if (maze[y][x] == 2) { // tool
                DrawRectangle(drawX, drawY, TILE_SIZE, TILE_SIZE, YELLOW);   
                DrawText("Tool", drawX + 13, drawY + 27, 20, BLACK); // X for horizontal, Y for vertical
                DrawRectangleLines(drawX, drawY, TILE_SIZE, TILE_SIZE, WHITE);
            } else if (maze[y][x] == 3) { // core device
                DrawRectangle(drawX, drawY, TILE_SIZE, TILE_SIZE, RED);      
                DrawText("Core\nDevice", drawX + 5, drawY + 20, 18, WHITE); // X for horizontal, Y for vertical
                DrawRectangleLines(drawX, drawY, TILE_SIZE, TILE_SIZE, WHITE);
            } else if (maze[y][x] == 4) { // starting point
                DrawRectangle(drawX, drawY, TILE_SIZE, TILE_SIZE, BLUE);
                DrawRectangleLines(drawX, drawY, TILE_SIZE, TILE_SIZE, WHITE);
            } else { // path
                DrawRectangle(drawX, drawY, TILE_SIZE, TILE_SIZE, LIGHTGRAY);
                DrawRectangleLines(drawX, drawY, TILE_SIZE, TILE_SIZE, WHITE);
            }
        }
    }

    // 繪製玩家
    DrawRectangle(offsetX + playerX * TILE_SIZE + 15, 
                  offsetY + playerY * TILE_SIZE + 15, 
                  TILE_SIZE - 30, TILE_SIZE - 30, DARKGRAY);

    // UI 資訊
    DrawText(TextFormat("Remaining Oxygen: %.1f s", timeLeft), 20, 20, 30, (timeLeft < 15) ? RED : WHITE);
    if (hasHandle) DrawText("Staus: Tool founded, please go to the Core Device.", 20, 60, 20, GREEN);
    else DrawText("Status: Please search for the tool in the North East corner.", 20, 60, 25, GRAY);

    // --- 繪製密碼輸入介面 ---
    if (showLockUI) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.8f));
        DrawText("Core Device Activate Program", GetScreenWidth()/2 - 150, 200, 40, WHITE);
        DrawText("Enter (U/D/L/R)", GetScreenWidth()/2 - 160, 280, 20, LIGHTGRAY);
        DrawText(TextFormat("Current input: %s", inputBuffer), GetScreenWidth()/2 - 100, 350, 40, YELLOW);
        DrawText("Automatically cleared if wrong (Press ESC to escape the device)", GetScreenWidth()/2 - 200, 450, 20, GRAY);
    }

    // --- 繪製故事開場白 (疊在最上層) ---
    if (currentState == L3_STORY) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.7f)); // 半透明黑底
        int boxWidth = 600;
        int boxHeight = 200;
        int boxX = (GetScreenWidth() - boxWidth) / 2;
        int boxY = (GetScreenHeight() - boxHeight) / 2;
        
        DrawRectangle(boxX, boxY, boxWidth, boxHeight, BLACK);
        DrawRectangleLines(boxX, boxY, boxWidth, boxHeight, WHITE);
        DrawText("Warning!", boxX + 50, boxY + 30, 40, RED);
        DrawText("Oxygen levels insufficient...\nPlease evacuate immediately.",
                 boxX + 50, boxY + 80, 30, WHITE);
        DrawText("[Press Space To Start]", boxX + 160, boxY + 160, 20, LIGHTGRAY);
    }
}