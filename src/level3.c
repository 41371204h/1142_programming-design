// TODO: add a rectangle that shows the player gets the key after entering the right answer

#include "raylib.h"
#include "game_shared.h"
#include "level3.h"
#include <string.h>

// --- 關卡設定 ---
#define TILE_SIZE 85
#define MAZE_COLS 8
#define MAZE_ROWS 8

// 定義第三關的內部狀態
typedef enum {
    L3_STORY,   // 顯示故事開場白
    L3_PLAYING, // 實際遊玩與計時中
    L3_FAILED   // 失敗畫面黑掉
} L3State;

// 迷宮初始模板 (每次重置都會拷貝這份)
// 0=path, 1=wall, 2=handle(North East), 3=Core device(center), 4=starting point
const int defaultMaze[MAZE_ROWS][MAZE_COLS] = {
    {1, 1, 1, 1, 1, 1, 1, 1},
    {1, 1, 1, 1, 0, 0, 0, 1},
    {1, 0, 0, 0, 0, 1, 0, 1},
    {1, 0, 1, 1, 2, 1, 0, 1},
    {1, 0, 0, 1, 1, 1, 0, 1},
    {1, 0, 1, 0, 0, 0, 0, 1},
    {1, 4, 1, 3, 1, 0, 1, 1},
    {1, 1, 1, 1, 1, 1, 1, 1}
};

// --- 關卡狀態變數 ---
static L3State currentState = L3_STORY;
static int maze[MAZE_ROWS][MAZE_COLS];
static int playerX = 1;      
static int playerY = 6;      // 讓玩家出生在左下角
static float timeLeft = 60.0f; ///
static bool hasHandle = false; 
static bool showLockUI = false; 
static char inputBuffer[5] = ""; 
static int inputIndex = 0;

// 控制「獲得道具」視窗是否顯示的開關
static bool showItemPopup = false;

// 紀錄在失敗畫面停留了多久
static float failedTimer = 0.0f;
// 用來控制是否顯示失敗故事頁面的變數
static bool showFailedStory = false;

// --- 重置關卡的專屬函數 ---
static void ResetLevel3(GameState *state) {
    currentState = L3_STORY; 
    playerX = 1;
    playerY = 6;
    timeLeft = 60.0f; ///
    hasHandle = false;
    showLockUI = false;
    showItemPopup = false;
    inputIndex = 0;
    memset(inputBuffer, 0, sizeof(inputBuffer));
    
    // 每次重置關卡時，也要把失敗計時器歸零
    failedTimer = 0.0f;
    showFailedStory = false; // 重置時關閉故事畫面
    
    for (int i = 0; i < state->inventory.count; i++) {
        if (strcmp(state->inventory.items[i], "Handle") == 0) {
            // 標準的 C 語言陣列刪除：將被刪除道具後面的所有物品全部往前移一格
            for (int j = i; j < state->inventory.count - 1; j++) {
                strcpy(state->inventory.items[j], state->inventory.items[j + 1]);
            }
            state->inventory.count--;      // 物品總數量減 1
            state->inventory.selected = 0; // 重置物品欄的游標，防止指到空白處
            break; // 找到並刪除後即可跳出迴圈
        }
    }

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
        ResetLevel3(state);
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
            if (showFailedStory) {
                // 第一階段 (故事)：計時器繼續跑，用來推動打字機效果
                failedTimer += GetFrameTime(); 
                
                // 玩家按下 X 鍵，結束故事頁面，進入系統關閉畫面
                if (IsKeyPressed(KEY_X)) {
                    showFailedStory = false; 
                }
            } else {
                // 第二階段 (系統關閉)：等待玩家重新開始或回主畫面
                if (IsKeyPressed(KEY_SPACE)) {
                    ResetLevel3(state);
                } 
                /*else if (IsKeyPressed(KEY_ESCAPE)) {
                    ResetLevel3(state);
                    state->currentScreen = SCREEN_HUB;
                } */
            }
            break;

        case L3_PLAYING:
            // 如果正在顯示彈出視窗，等待玩家確認
            if (showItemPopup) {
                if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER)) {
                    showItemPopup = false;
                    
                    // 玩家按空白鍵確認後，才真正判定第三關過關並回主畫面！
                    state->isLevel3Cleared = true;
                    state->currentScreen = SCREEN_HUB; 
                    ResetLevel3(state); 
                }
                return; // 暫停時間倒數與背景的一切活動
            }
            timeLeft -= GetFrameTime();
            if (timeLeft <= 0) {
                timeLeft = 0;
                currentState = L3_FAILED; 
                failedTimer = 0.0f; // 確保剛進入失敗狀態時，計時器是 0
                showFailedStory = true;
                break;
            }
            // make sure that the timer is still counting down when the inventory is opened
            if (state->inventory.opened) return;

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
                        showLockUI = false;   // 關閉密碼鎖的 UI
                        showItemPopup = true; // 開啟獲得道具視窗
                        AddItem(state, "Key Card");
                        // ResetLevel3(state); // 為下一次遊玩重置
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
                        AddItem(state, "Handle"); // put handle into the inventory
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
void DrawLevel3(const GameState *state) {
    // 如果是失敗狀態，畫出兩階段的黑屏
    if (currentState == L3_FAILED) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);
        
        if (showFailedStory) {
            // 第一階段：顯示故事文字 (套用自訂字體)
            // 第一行字直接完整顯示
            DrawTextEx(state->storyFont, "You hear a familiar voice saying: ", 
                       (Vector2){GetScreenWidth()/2.0f - 580, GetScreenHeight()/2.0f - 280}, 
                       35, 2, GRAY);
            
            // 打字機特效邏輯：
            // 根據停留的時間 (failedTimer) 乘上每秒想出現的字數 (例如 20)
            const char *typeText = "\"Wake up! You're almost there.\nDon't fall asleep...\"";
            int charsToShow = (int)(failedTimer * 20.0f); 
            
            // 第二行字套用 TextSubtext 慢慢浮現
            DrawTextEx(state->storyFont, TextSubtext(typeText, 0, charsToShow),
                       (Vector2){GetScreenWidth()/2.0f - 580, GetScreenHeight()/2.0f - 80}, 
                       50, 2, WHITE);
            
            // 提示玩家按 X 鍵
            DrawText("[ Press X to Continue ]", GetScreenWidth()/2 - 130, GetScreenHeight() - 100, 20, DARKGRAY);

        } else {
            // 第二階段：顯示系統關閉與按鍵指示 (這裡保持原本乾淨俐落的系統字體)
            DrawText("SYSTEM SHUTDOWN", GetScreenWidth()/2 - 380, 350, 80, RED);
            DrawText("[ Press SPACE to Restart Level 3 ]", GetScreenWidth()/2 - 250, 550, 30, WHITE);
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
            } 
            else if (maze[y][x] == 2) { // handle
                // 設定原始圖片的來源範圍與目標範圍
                Rectangle sourceRec = { 0.0f, 0.0f, (float)state->handleSprite.width, (float)state->handleSprite.height };
                Rectangle destRec = { drawX, drawY, TILE_SIZE, TILE_SIZE };
                
                // 繪製 Handle 貼圖，會自動縮放適應 TILE_SIZE
                DrawTexturePro(state->handleSprite, sourceRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);
            } 
            else if (maze[y][x] == 3) { // core device
                DrawRectangle(drawX, drawY, TILE_SIZE, TILE_SIZE, RED);      
                DrawText("Core\nDevice", drawX + 5, drawY + 20, 18, WHITE); // X for horizontal, Y for vertical
            } 
            else if (maze[y][x] == 4) { // starting point
                DrawRectangle(drawX, drawY, TILE_SIZE, TILE_SIZE, DARKGRAY);
            } 
            else { // path
                DrawRectangle(drawX, drawY, TILE_SIZE, TILE_SIZE, LIGHTGRAY);
            }
        }
    }
    // 繪製整個迷宮的最外圍邊框
    // 寬度 = 行數 * 格子大小；高度 = 列數 * 格子大小
    Rectangle mazeOuterBounds = { 
        (float)offsetX, 
        (float)offsetY, 
        (float)(MAZE_COLS * TILE_SIZE), 
        (float)(MAZE_ROWS * TILE_SIZE) 
    };
    
    // 畫一個粗細為 4.0 的白色大外框，把整個迷宮包起來
    DrawRectangleLinesEx(mazeOuterBounds, 4.0f, WHITE);

    // 繪製玩家
    Rectangle sourceRec = { 0.0f, 0.0f, (float)state->playerSprite.width, (float)state->playerSprite.height };
    Rectangle destRec = { offsetX + playerX * TILE_SIZE, offsetY + playerY * TILE_SIZE, TILE_SIZE, TILE_SIZE };
    DrawTexturePro(state->playerSprite, sourceRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);

    // UI 資訊
    DrawText(TextFormat("Remaining Time: %.1f s", timeLeft), 20, 20, 30, (timeLeft < 15) ? RED : WHITE);
    if (hasHandle) DrawText("Staus: Handle founded, please go to the Core Device.", 20, 60, 25, GREEN);
    else DrawText("Hint: Please search for the handle in the North East corner.", 20, 60, 25, GRAY);

    // --- 繪製密碼輸入介面 ---
    if (showLockUI) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.8f));
        DrawText("Core Device Activate Program", GetScreenWidth()/2 - 260, 200, 40, WHITE);
        DrawText("Enter (U/D/L/R) using arrow keys", GetScreenWidth()/2 - 260, 280, 20, LIGHTGRAY);
        DrawText(TextFormat("Current input: %s", inputBuffer), GetScreenWidth()/2 - 200, 350, 40, YELLOW);
        DrawText("Automatically cleared if wrong", GetScreenWidth()/2 - 260, 450, 20, GRAY);
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

    // 繪製過關獲得道具的彈出視窗 (疊在絕對的最上層)
    if (showItemPopup) {
        // 畫全螢幕半透明黑底 (跟故事開場白一樣的風格)
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.8f)); 
        
        // 視窗大小與置中計算
        int boxWidth = 500;
        int boxHeight = 350;
        int boxX = (GetScreenWidth() - boxWidth) / 2;
        int boxY = (GetScreenHeight() - boxHeight) / 2;
        
        // 畫出如同第三關開場白那樣的純黑底框與白線
        DrawRectangle(boxX, boxY, boxWidth, boxHeight, BLACK);
        DrawRectangleLinesEx((Rectangle){boxX, boxY, boxWidth, boxHeight}, 3.0f, WHITE);
        
        // 顯示獲得道具文字
        DrawText("SYSTEM UNLOCKED", boxX + 90, boxY + 30, 35, GREEN);
        DrawText("Obtained: Key Card", boxX + 110, boxY + 230, 22, WHITE);
        DrawText("[ Press SPACE to Return to Main Hub ]", boxX + 60, boxY + 300, 18, LIGHTGRAY);

        // 渲染道具圖片 (置中縮放為 100x100)
        // (若你有專屬的過關道具圖片，請將 handleSprite 替換為你的圖片變數)
        Rectangle sourceRec = { 0.0f, 0.0f, (float)state->keySprite.width, (float)state->keySprite.height };
        Rectangle destRec = { boxX + (boxWidth - 100) / 2, boxY + 100, 100, 100 };
        DrawTexturePro(state->keySprite, sourceRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);
    }
}