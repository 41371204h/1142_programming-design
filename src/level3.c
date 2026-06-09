#include "raylib.h"
#include "game_shared.h"
#include "level3.h"
#include "audio.h"
#include <SDL2/SDL_mixer.h>
#include <string.h>

// --- 關卡設定 ---
#define TILE_SIZE 85
#define MAZE_COLS 6
#define MAZE_ROWS 6

// 定義第三關的內部狀態
typedef enum {
    L3_STORY,   // 顯示故事開場白
    L3_PLAYING, // 實際遊玩與計時中
    L3_FAILED   // 失敗畫面黑掉
} L3State;

// 迷宮初始模板 (每次重置都會拷貝這份)
// 0=path, 1=wall, 2=handle, 3=Core device(center), 4=starting point
const int defaultMaze[MAZE_ROWS][MAZE_COLS] = {
    {1, 1, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 0},
    {0, 1, 1, 2, 1, 0},
    {0, 0, 1, 1, 1, 0},
    {0, 1, 0, 0, 0, 0},
    {0, 1, 3, 1, 0, 1},
};

// --- 關卡狀態變數 ---
static L3State currentState = L3_STORY;
static int maze[MAZE_ROWS][MAZE_COLS];
static int playerX = 1;      
static int playerY = 6;      // 讓玩家出生在左下角
static float timeLeft = 60.0f; ///
static bool hasHandle = false; 
static bool showLockUI = false; 
static char inputBuffer[20] = ""; 
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
    playerX = 0;
    playerY = 5;
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
            // 💡 1. 核心優化：只要一進來 L3_STORY 狀態（畫面一跳出來、Warning還在時）
            // 只要警報還沒響，就利用一個小計時器或是每影偵測讓它持續響起
            // 為了防止每一影都重複呼叫疊音，我們可以利用 timeLeft 剛好是 60 秒的時機點來做「只播放一次」的觸發：
            if (timeLeft == 60.0f && !showLockUI) {
                play_effect_alarm(); 
            }

            // 等待玩家按下空白鍵開始遊戲
            if (IsKeyPressed(KEY_SPACE)) {
                currentState = L3_PLAYING;
                
                // 💡 2. 核心優化：按下空白鍵（Warning 提示框消失）的瞬間
                // 呼叫 SDL_mixer 的強制截斷指令，把正在狂響的警報聲立刻關掉！
                // 這裡直接使用標準的 SDL_mixer API 來停掉所有音效通道
                Mix_HaltChannel(-1); 
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
                    play_bgm(3); 
                } 
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

                // 當時間到失敗的那一瞬間，立刻淡出關閉背景音樂 bgm3.mp3
                stop_bgm(); 
                break;
            }
            // make sure that the timer is still counting down when the inventory is opened
            if (state->inventory.opened) return;

            // 2. 密碼輸入邏輯
            if (showLockUI) {
                if (IsKeyPressed(KEY_UP) && inputIndex < 19) inputBuffer[inputIndex++] = 'U';
                if (IsKeyPressed(KEY_DOWN) && inputIndex < 19) inputBuffer[inputIndex++] = 'D';
                if (IsKeyPressed(KEY_LEFT) && inputIndex < 19) inputBuffer[inputIndex++] = 'L';
                if (IsKeyPressed(KEY_RIGHT) && inputIndex < 19) inputBuffer[inputIndex++] = 'R';

                if (inputIndex == 19) {
                    inputBuffer[19] = '\0';
                    if (strcmp(inputBuffer, state->secretSequence) == 0) {
                        // 密碼正確！過關
                        showLockUI = false;   // 關閉密碼鎖的 UI
                        showItemPopup = true; // 開啟獲得道具視窗
                        
                        // 最終闖關成功（解開程式拿到 Card Key 鑰匙）時，播放 success.wav
                        play_effect_success();

                        AddItem(state, "Card Key");
                    } else {
                        // 密碼錯誤，清空重打 (且播放失敗警示音)
                        play_effect_fail();
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
                        
                        // 走迷宮拿到拉柄工具後，播放 get_tool.wav 音效
                        play_effect_get_tool();

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
            DrawTextEx(state->storyFont, "You hear a familiar voice saying: ", 
                       (Vector2){GetScreenWidth()/2.0f - 580, GetScreenHeight()/2.0f - 280}, 
                       35, 2, GRAY);
            
            const char *typeText = "\"Wake up! You're almost there.\nDon't fall asleep...\"";
            int charsToShow = (int)(failedTimer * 20.0f); 
            
            DrawTextEx(state->storyFont, TextSubtext(typeText, 0, charsToShow),
                       (Vector2){GetScreenWidth()/2.0f - 580, GetScreenHeight()/2.0f - 80}, 
                       54, 2, WHITE);
            
            DrawText("[ Press X to Continue ]", GetScreenWidth()/2 - 130, GetScreenHeight() - 100, 20, DARKGRAY);

        } else {
            DrawText("SYSTEM SHUTDOWN", GetScreenWidth()/2 - 380, 350, 80, RED);
            DrawText("[ Press SPACE to Restart Level 3 ]", GetScreenWidth()/2 - 250, 550, 30, WHITE);
        }
        return; 
    }
    

    // --- 繪製迷宮與遊戲本體 ---
    int offsetX = (GetScreenWidth() - (MAZE_COLS * TILE_SIZE)) / 2; // 置中顯示
    int offsetY = 180;

    for (int y = 0; y < MAZE_ROWS; y++) {
        for (int x = 0; x < MAZE_COLS; x++) {
            int drawX = offsetX + x * TILE_SIZE;
            int drawY = offsetY + y * TILE_SIZE;

            // 設定目標貼圖的渲染範圍（自動縮放至每一格 TILE_SIZE 的大小）
            Rectangle destRec = { (float)drawX, (float)drawY, (float)TILE_SIZE, (float)TILE_SIZE };
            Rectangle sourceRec = { 0.0f, 0.0f, (float)0, (float)0 };

            // 根據不同的迷宮代碼，指定對應的圖片與來源裁剪範圍
            switch (maze[y][x]) {
                case 0: // path.png
                    sourceRec = (Rectangle){ 0.0f, 0.0f, (float)state->pathSprite.width, (float)state->pathSprite.height };
                    DrawTexturePro(state->pathSprite, sourceRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);
                    break;

                case 1: // wall.png
                    sourceRec = (Rectangle){ 0.0f, 0.0f, (float)state->wallSprite.width, (float)state->wallSprite.height };
                    DrawTexturePro(state->wallSprite, sourceRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);
                    break;

                case 2: // wall_tool.png (原本的 handle)
                    sourceRec = (Rectangle){ 0.0f, 0.0f, (float)state->wallToolSprite.width, (float)state->wallToolSprite.height };
                    DrawTexturePro(state->wallToolSprite, sourceRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);
                    break;

                case 3: // wall_device.png (原本的 Core device)
                    sourceRec = (Rectangle){ 0.0f, 0.0f, (float)state->wallDeviceSprite.width, (float)state->wallDeviceSprite.height };
                    DrawTexturePro(state->wallDeviceSprite, sourceRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);
                    break;
            }
        }
    }
    
    // 繪製整個迷宮的最外圍邊框
    Rectangle mazeOuterBounds = { 
        (float)offsetX, 
        (float)offsetY, 
        (float)(MAZE_COLS * TILE_SIZE), 
        (float)(MAZE_ROWS * TILE_SIZE) 
    };
    DrawRectangleLinesEx(mazeOuterBounds, 4.0f, BROWN);

    // 繪製玩家
    Rectangle pSourceRec = { 0.0f, 0.0f, (float)state->playerSprite.width, (float)state->playerSprite.height };
    Rectangle pDestRec = { offsetX + playerX * TILE_SIZE, offsetY + playerY * TILE_SIZE, TILE_SIZE, TILE_SIZE };
    DrawTexturePro(state->playerSprite, pSourceRec, pDestRec, (Vector2){0, 0}, 0.0f, WHITE);

    // UI 資訊
    DrawText(TextFormat("Remaining Time: %.1f s", timeLeft), 20, 20, 30, (timeLeft < 15) ? RED : WHITE);
    if (hasHandle) DrawText("Staus: Handle founded, please go to the Core Device.", 20, 60, 25, GREEN);
    else DrawText("Hint: Do you remember the message and directional clues you found in Level 1?", 20, 60, 25, GRAY);

    // 💡 核心新增：在第三關迷宮右上角/右側空白處繪製說明欄 (Legend)
    // 假設你的迷宮繪製有定義 offsetX 和 offsetY，我們直接往迷宮右邊界算過去
    int legendX = offsetX + (MAZE_COLS * TILE_SIZE) + 60; // 迷宮右側外推 60 像素
    int legendY = offsetY + 20;                           // 與迷宮頂端對齊附近

    // 1. 繪製圖鑑外框與標題
    DrawRectangle(legendX - 20, legendY - 10, 320, 220, Fade(BLACK, 0.5f));
    DrawRectangleLines(legendX - 20, legendY - 10, 320, 220, GRAY);
    DrawText("FACILITY MAP LEGEND", legendX, legendY, 20, SKYBLUE);

    // 2. 繪製 Handle 的標示 (使用獨立的 handleDetailSprite)
    Rectangle hSrc = { 0.0f, 0.0f, (float)state->handleDetailSprite.width, (float)state->handleDetailSprite.height };
    Rectangle hDest = { (float)legendX, (float)legendY + 40, 60, 60 }; // 縮放為 60x60 大小
    DrawTexturePro(state->handleDetailSprite, hSrc, hDest, (Vector2){0, 0}, 0.0f, WHITE);
    DrawText("Handle", legendX + 80, legendY + 50, 22, WHITE);
    DrawText("TAKE IT", legendX + 80, legendY + 75, 15, GRAY); // 提示玩家這是該物件

    // 3. 繪製 Device 的標示 (使用獨立的 device02Sprite)
    Rectangle dSrc = { 0.0f, 0.0f, (float)state->device02Sprite.width, (float)state->device02Sprite.height };
    Rectangle dDest = { (float)legendX, (float)legendY + 120, 60, 60 }; // 縮放為 60x60 大小
    DrawTexturePro(state->device02Sprite, dSrc, dDest, (Vector2){0, 0}, 0.0f, WHITE);
    DrawText("Device", legendX + 80, legendY + 130, 22, WHITE);
    DrawText("GOAL", legendX + 80, legendY + 155, 15, GRAY);


    // --- 繪製密碼輸入介面 ---
    if (showLockUI) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.8f));
        DrawText("Core Device Activate Program", GetScreenWidth()/2 - 400, 200, 40, WHITE);
        DrawText("Enter the correct path to the maze (U/D/L/R)\nusing arrow keys", GetScreenWidth()/2 - 400, 280, 28, LIGHTGRAY);
        DrawText(TextFormat("Current input: %s", inputBuffer), GetScreenWidth()/2 - 400, 380, 40, YELLOW);
        DrawText("[Automatically cleared if wrong]", GetScreenWidth()/2 - 400, 470, 20, GRAY);
    }

    // --- 繪製故事開場白 (疊在最上層) ---
    if (currentState == L3_STORY) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.7f)); 
        int boxWidth = 600;
        int boxHeight = 200;
        int boxX = (GetScreenWidth() - boxWidth) / 2;
        int boxY = (GetScreenHeight() - boxHeight) / 2;
        
        DrawRectangle(boxX, boxY, boxWidth, boxHeight, BLACK);
        DrawRectangleLines(boxX, boxY, boxWidth, boxHeight, WHITE);
        DrawText("Warning!", boxX + 50, boxY + 30, 40, RED);
        DrawText("Oxygen levels insufficient...\nPlease evacuate immediately.", boxX + 50, boxY + 80, 30, WHITE);
        DrawText("[Press Space To Start]", boxX + 160, boxY + 160, 20, LIGHTGRAY);
    }

    // 💡 繪製過關獲得道具的彈出視窗 (在此加入框住鑰匙物品的長方形裝飾線)
    if (showItemPopup) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.8f)); 
        
        int boxWidth = 500;
        int boxHeight = 350;
        int boxX = (GetScreenWidth() - boxWidth) / 2;
        int boxY = (GetScreenHeight() - boxHeight) / 2;
        
        DrawRectangle(boxX, boxY, boxWidth, boxHeight, BLACK);
        DrawRectangleLinesEx((Rectangle){boxX, boxY, boxWidth, boxHeight}, 3.0f, WHITE);
        
        DrawText("SYSTEM UNLOCKED", boxX + 90, boxY + 30, 35, GREEN);
        DrawText("Obtained: Card Key", boxX + 150, boxY + 230, 22, WHITE);
        DrawText("[ Press SPACE to Return to Main Hub ]", boxX + 90, boxY + 300, 18, LIGHTGRAY);

        // 💡 核心功能：使用 DrawRectangleLinesEx 繪製一個圍繞在鑰匙外圍的科技感長方形亮框
        Rectangle itemOutlineRec = { boxX + (boxWidth - 120) / 2, boxY + 90, 120, 120 };
        DrawRectangleLinesEx(itemOutlineRec, 2.0f, LIME);

        Rectangle sourceRec = { 0.0f, 0.0f, (float)state->keySprite.width, (float)state->keySprite.height };
        Rectangle destRec = { boxX + (boxWidth - 100) / 2, boxY + 100, 100, 100 };
        DrawTexturePro(state->keySprite, sourceRec, destRec, (Vector2){0, 0}, 0.0f, WHITE);
    }
}