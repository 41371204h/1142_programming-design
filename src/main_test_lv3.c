#include "raylib.h"
#include "game_shared.h"
#include "level3.h" 
#include <string.h>

int main(void) {
    // 1. 初始化視窗 (假設解析度為 1280x720)
    InitWindow(1280, 960, "深海觀測站 - 第三關測試區");
    SetTargetFPS(60);

    // 2. 建立全域的「遊戲狀態存檔」
    GameState state = {0}; // 把所有變數預設為 0 / false
    
    // 【測試專用設定】
    state.currentScreen = SCREEN_LEVEL3;     // 直接把畫面切到第三關
    strcpy(state.secretSequence, "URLD");    // 假裝玩家已經在第一關拿到了密碼

    // 3. 遊戲主迴圈
    while (!WindowShouldClose()) {
        
        // --- 更新邏輯 ---
        if (state.currentScreen == SCREEN_LEVEL3) {
            UpdateLevel3(&state);
        } else if (state.currentScreen == SCREEN_HUB) {
            // 如果你在第三關過關了，state.currentScreen 會變成 SCREEN_HUB
            // 這裡可以寫個按鍵讓你按 R 再次回到第三關測試
            if (IsKeyPressed(KEY_R)) {
                state.currentScreen = SCREEN_LEVEL3;
            }
        }

        // --- 繪製畫面 ---
        BeginDrawing();
        ClearBackground(BLACK); // set the background color to black

        if (state.currentScreen == SCREEN_LEVEL3) {
            DrawLevel3();
        } else if (state.currentScreen == SCREEN_HUB) {
            DrawText("This is the main screen HUB", 320, 400, 40, WHITE);
            DrawText("Level 3 complete. Press R to restart.", 400, 480, 20, GRAY);
        }

        EndDrawing();
    }

    // 4. 關閉視窗與清理
    CloseWindow();
    return 0;
}