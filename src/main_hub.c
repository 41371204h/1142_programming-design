#include "raylib.h"
#include "game_shared.h"
#include "level3.h"
// #include "level1_2.h" ///
#include <string.h> /// for testing lv3

#define SCREEN_HEIGHT 960
#define SCREEN_WIDTH 1280

// Declare the variables for visual design
/* static Texture2D texHubBg;
static Texture2D texTermOffline;
static Texture2D texTermReady;
static Texture2D texTermCleared; */

// Homw page terminal settings
static Rectangle termLevel1 = { 150, 450, 250, 180 }; // terminal for Level 1 (left)
static Rectangle termLevel2 = { 515, 450, 250, 180 }; // terminal for Level 2 (right)
static Rectangle termLevel3 = { 880, 450, 250, 180 }; // terminal for Level 3 (middle)

// Declare the functions only used in home page
void UpdateHub(GameState *state);
void DrawHub(GameState *state);

int main(void) {
    // 1. Initializing the game window (res: 1280x960)
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Puzzle Game"); ///
    SetTargetFPS(60);

    // 2. Initializing the game's core state 
    GameState state = {0};
    state.currentScreen = SCREEN_HUB; // the game starts at home page
    
    /// to test the logic of unlocking levels, modify the flag to true
    state.isLevel1Cleared = false; 
    state.isLevel2Cleared = false;

    // 3. Game main loop
    while (!WindowShouldClose()) {
        
        // --- A. 邏輯更新層 (Update) ---
        switch (state.currentScreen) {
            case SCREEN_HUB:
                UpdateHub(&state);
                break;
            case SCREEN_LEVEL1:
                // UpdateLevel1(&state); // 等隊友完成後串接 ///
                // 暫時用按 Enter 鍵假裝過關回到主畫面
                if (IsKeyPressed(KEY_ENTER)) {
                    state.isLevel1Cleared = true;
                    strcpy(state.secretSequence, "URLD"); /// for testing lv3
                    state.currentScreen = SCREEN_HUB;
                }
                break;
            case SCREEN_LEVEL2:
                // UpdateLevel2(&state); // 等隊友完成後串接 ///
                if (IsKeyPressed(KEY_ENTER)) {
                    state.isLevel2Cleared = true;
                    state.currentScreen = SCREEN_HUB;
                }
                break;
            case SCREEN_LEVEL3:
                UpdateLevel3(&state); // 執行你已經寫好的第三關
                break;
            case SCREEN_ENDING:
                if (IsKeyPressed(KEY_SPACE)) state.currentScreen = SCREEN_HUB; // 結局看完按空白鍵重開
                break;
        }

        // --- B. 畫面繪製層 (Draw) ---
        BeginDrawing();
        ClearBackground(BLACK); // 基礎底色

        switch (state.currentScreen) {
            case SCREEN_HUB:
                DrawHub(&state);
                break;
            case SCREEN_LEVEL1:
                DrawText("--- Level 1 Communications Control Hub---", 400, 400, 30, LIGHTGRAY); ///
                DrawText("[Under developement] Press ENTER to pretend clearing lv1 and repair communication", 350, 500, 20, YELLOW);
                break;
            case SCREEN_LEVEL2:
                DrawText("--- Level 2 Maintenance Hub---", 400, 400, 30, LIGHTGRAY); ///
                DrawText("[Under developement] Press ENTER to pretend clearing lv2 and repair the maintenance system", 320, 500, 20, YELLOW);
                break;
            case SCREEN_LEVEL3:
                DrawLevel3(); // 執行你寫好的第三關畫面
                break;
            case SCREEN_ENDING:
                DrawText("Congratulations. You have completed the mission.\nThe core was repaired safely and the escape pod has been launched.", 300, 400, 35, GREEN);
                DrawText("[ Press SPACE to go bake to Main Hub ]", 480, 550, 20, GRAY);
                break;
        }

        EndDrawing();
    }

    // 4. 清理並關閉
    CloseWindow();
    return 0;
}

// --------------------------------------------------------
// 主畫面 HUB 的邏輯更新
// --------------------------------------------------------
void UpdateHub(GameState *state) {
    // 獲取目前滑鼠在視窗中的 X, Y 座標
    Vector2 mousePos = GetMousePosition();

    // 只有在滑鼠按下左鍵時，才去檢查碰到了哪台電腦
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        
        // 檢查第一關按鈕 (永遠開放)
        if (CheckCollisionPointRec(mousePos, termLevel1)) {
            state->currentScreen = SCREEN_LEVEL1;
        }
        
        // 檢查第二關按鈕 (必須先過第一關)
        if (CheckCollisionPointRec(mousePos, termLevel2)) {
            if (state->isLevel1Cleared) {
                state->currentScreen = SCREEN_LEVEL2;
            }
        }
        
        // 檢查第三關按鈕 (必須先過第二關)
        if (CheckCollisionPointRec(mousePos, termLevel3)) {
            if (state->isLevel2Cleared) {
                state->currentScreen = SCREEN_LEVEL3;
            }
        }
    }
}

// --------------------------------------------------------
// 主畫面 HUB 的畫面繪製
// --------------------------------------------------------
void DrawHub(GameState *state) {
    DrawText("Main Hub", 450, 150, 40, WHITE);
    DrawText("Click on the terminal to repair the system", 480, 230, 20, LIGHTGRAY);

    Vector2 mousePos = GetMousePosition();

    // ---- 繪製第一台終端機 (通訊) ----
    bool hover1 = CheckCollisionPointRec(mousePos, termLevel1);
    Color color1 = state->isLevel1Cleared ? GREEN : (hover1 ? SKYBLUE : BLUE);
    DrawRectangleRec(termLevel1, color1);
    DrawRectangleLinesEx(termLevel1, 3, WHITE); /// symbolizing lv1 terminal
    DrawText("1. Communications Section", termLevel1.x + 50, termLevel1.y + 50, 24, WHITE);
    DrawText(state->isLevel1Cleared ? "[ Repaired ]" : "[ To be repaired ]", termLevel1.x + 60, termLevel1.y + 110, 20, state->isLevel1Cleared ? LIME : YELLOW);

    // ---- 繪製第二台終端機 (維修) ----
    bool hover2 = CheckCollisionPointRec(mousePos, termLevel2);
    Color color2;
    if (!state->isLevel1Cleared) color2 = DARKGRAY; // 未解鎖
    else color2 = state->isLevel2Cleared ? GREEN : (hover2 ? SKYBLUE : BLUE);
    
    DrawRectangleRec(termLevel2, color2);
    DrawRectangleLinesEx(termLevel2, 3, WHITE);
    DrawText("2. Maintenance Section", termLevel2.x + 50, termLevel2.y + 50, 24, state->isLevel1Cleared ? WHITE : GRAY);
    if (!state->isLevel1Cleared) {
        DrawText("[ System offline ]", termLevel2.x + 60, termLevel2.y + 110, 20, RED);
    } else {
        DrawText(state->isLevel2Cleared ? "[ Repaired ]" : "[ To be repaired ]", termLevel2.x + 60, termLevel2.y + 110, 20, state->isLevel2Cleared ? LIME : YELLOW);
    }

    // ---- 繪製第三台終端機 (動力核心) ----
    bool hover3 = CheckCollisionPointRec(mousePos, termLevel3);
    Color color3;
    if (!state->isLevel2Cleared) color3 = DARKGRAY; // 未解鎖
    else color3 = state->isLevel3Cleared ? GREEN : (hover3 ? SKYBLUE : BLUE);

    DrawRectangleRec(termLevel3, color3);
    DrawRectangleLinesEx(termLevel3, 3, WHITE);
    DrawText("3. Power Section", termLevel3.x + 50, termLevel3.y + 50, 24, state->isLevel2Cleared ? WHITE : GRAY);
    if (!state->isLevel2Cleared) {
        DrawText("[ Permission denied ]", termLevel3.x + 60, termLevel3.y + 110, 20, RED);
    } else {
        DrawText(state->isLevel3Cleared ? "[ Repaired ]" : "[ To be repaired ]", termLevel3.x + 60, termLevel3.y + 110, 20, state->isLevel3Cleared ? LIME : YELLOW);
    }

    // ---- 全通關結局條件 ----
    if (state->isLevel1Cleared && state->isLevel2Cleared && state->isLevel3Cleared) {
        DrawRectangle(0, 820, SCREEN_WIDTH, 140, DARKGREEN);
        DrawText("[ Warning! ] All systems have been repaired! The escape pod has been unlocked!", 20, 850, 30, WHITE);
        DrawText("Press SPACE to start the launch procedure", 420, 900, 23, LIGHTGRAY);
        if (IsKeyPressed(KEY_SPACE)) {
            state->currentScreen = SCREEN_ENDING;
        }
    }
}