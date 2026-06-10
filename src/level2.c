#include "raylib.h"
#include "game_shared.h"
#include "level2.h"
#include "audio.h"
#include <string.h>
#include <stdbool.h>

// 💡 調整拼圖大小：將 TILE_SIZE 放大至 160，讓拼圖佔滿畫面的 8 成
#define PUZZLE_TILE_SIZE 160 
#define PLAYER_SIZE 70
#define PLAYER_SPEED 4

static Vector2 playerPos;

// 💡 將 paperPile 的位置往下移（Y 從 350 改到 550）
static Rectangle paperPile = {540, 670, 300, 230};
static Rectangle doorRect = {565, 800, 200, 200};

// 💡 定義多個檔案室專屬的「垃圾干擾物/環境裝飾」碰撞體
static Rectangle fakeShelfDecor = { 100, 300, 100, 400 }; // 左邊大書架區
static Rectangle fakeLoosePaper = { 950, 760, 120, 60 };  // 右下角散落的紙張
static Rectangle fakeWallPanel  = { 400, 250, 120, 120 }; // 中央牆壁暗處

// ---------------------------
// 💡 核心新增：牆面隨機亂拼文字的數據結構
// ---------------------------
typedef struct {
    char character[2]; // 儲存單個字元字串 (例如 "U\0")
    Vector2 pos;       // 文字在牆面上的 X, Y 座標
    float rotation;    // 旋轉角度 (0 ~ 180 度)
    float scale;       // 微幅隨機縮放大小，讓字體看起來更生動
} ScatteredText;

static ScatteredText wallTextList[20];
static const char *secretSequenceStr = "UUUURRRDUURRDDDDLLLD";
static bool isScatteredTextInitialized = false;

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
// 对话
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
    "Strange...\nI can't even remember the internal structure\nof the escape zone.",
    "This won't do,\nI'll have to look for it."
};

static const char *puzzleOpenDialogue[] = {
    "These papers are all damaged...",
    "I need to fix it,\nbut how could I know which piece goes where?"
};

static const char *levelCompleteDialogue[] = {
    "That's it!\nIt is  the map of the escape zone!",
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
// 初始化拼图
// ---------------------------
void InitPuzzle(void)
{
    int temp[9] = {
        4,7,2,
        8,3,5,
        1,9,6
    };

    for (int i = 0; i < 9; i++)
    {
        puzzle[i] = temp[i];
    }
}

// ---------------------------
// 判断拼图完成
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
    playerPos = (Vector2){1100, 450};
    showPuzzle = false;
    showMap = false;
    showDialogue = false;
    returnToHubAfterDialogue = false;
    selectedTile = -1;
    cursorX = 0;
    cursorY = 0;
    StartDialogue(enterDialogue, sizeof(enterDialogue) / sizeof(enterDialogue[0]), L2_DIALOGUE_NONE);

    InitPuzzle();

    // 💡 核心新增：隨機生成 20 個字元在中央金屬牆面上的凌亂排布數據
    for (int i = 0; i < 20; i++) {
        wallTextList[i].character[0] = secretSequenceStr[i];
        wallTextList[i].character[1] = '\0';
        
        // 限制在中央鐵板的大致範圍內（X: 180~950, Y: 220~500）
        // 利用橫座標隨字元索引遞增（加上大量隨機擾動），讓字體能保持「從左到右」的閱讀順序，但排得歪七扭八
        wallTextList[i].pos.x = 430.0f + (i * 28.0f) + (float)GetRandomValue(-8, 8);
        wallTextList[i].pos.y = 380.0f + (float)GetRandomValue(-40, 70);
        
        // 根據你的要求：文字可以有些轉個角度 (0 到 180 度都有)
        wallTextList[i].rotation = (float)GetRandomValue(0, 180);
        
        // 字體大小隨機微調
        wallTextList[i].scale = (float)GetRandomValue(32, 42);
    }
    isScatteredTextInitialized = true;
}

// ---------------------------
// 玩家移動
// ---------------------------
void UpdatePlayer(void)
{
    if (IsKeyDown(KEY_UP))    playerPos.y -= PLAYER_SPEED;
    if (IsKeyDown(KEY_DOWN))  playerPos.y += PLAYER_SPEED;
    if (IsKeyDown(KEY_LEFT))  playerPos.x -= PLAYER_SPEED;
    if (IsKeyDown(KEY_RIGHT)) playerPos.x += PLAYER_SPEED;
}

// ---------------------------
// 更新拼圖
// ---------------------------
void UpdatePuzzle(GameState *state)
{
    if (IsKeyPressed(KEY_UP) && cursorY > 0)    cursorY--;
    if (IsKeyPressed(KEY_DOWN) && cursorY < 2)  cursorY++;
    if (IsKeyPressed(KEY_LEFT) && cursorX > 0)   cursorX--;
    if (IsKeyPressed(KEY_RIGHT) && cursorX < 2)  cursorX++;

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
                    
                    play_effect_win();

                    StartDialogue(levelCompleteDialogue, sizeof(levelCompleteDialogue) / sizeof(levelCompleteDialogue[0]), L2_DIALOGUE_COMPLETE);

                    state->isLevel2Cleared = true;
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

    if (IsKeyPressed(KEY_Z)) {
        if (CheckCollisionRecs(playerRect, fakeShelfDecor)) {
            static const char *txt[] = { "Rows and rows of old management logs...", "Most of them are completely ruined by moisture." };
            StartDialogue(txt, 2, L2_DIALOGUE_NONE);
            return;
        }
        if (CheckCollisionRecs(playerRect, fakeLoosePaper)) {
            static const char *txt[] = { "Just some torn blueprints.", "Nothing related to the escape zone map." };
            StartDialogue(txt, 2, L2_DIALOGUE_NONE);
            return;
        }
        if (CheckCollisionRecs(playerRect, fakeWallPanel)) {
            static const char *txt[] = { "There's a hollow metallic sound when I knock on this wall...", "But there's no way to open it right now." };
            StartDialogue(txt, 2, L2_DIALOGUE_NONE);
            return;
        }
    }

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
// 繪製拼圖
// ---------------------------
void DrawPuzzle(const GameState *state)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), BLACK);

    int totalSize = 3 * PUZZLE_TILE_SIZE; 
    int startX = (GetScreenWidth() - totalSize) / 2;
    int startY = (GetScreenHeight() - totalSize) / 2 - 40; 

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

            if (tileValue >= 1 && tileValue <= 9) {
                Rectangle srcRec = { 0.0f, 0.0f, (float)state->puzzleParts[tileValue - 1].width, (float)state->puzzleParts[tileValue - 1].height };
                DrawTexturePro(state->puzzleParts[tileValue - 1], srcRec, tileRect, (Vector2){0, 0}, 0.0f, WHITE);
            }

            if (index == selectedTile)
            {
                DrawRectangleLinesEx(tileRect, 5.0f, GREEN);
            }

            if (x == cursorX && y == cursorY)
            {
                DrawRectangleLinesEx(tileRect, 5.0f, SKYBLUE);
                DrawRectangleRec(tileRect, Fade(WHITE, 0.15f));
            }
        }
    }

    const char *line1 = "Select two puzzle pieces to swap their positions.";
    const char *line2 = "Arrow Keys: Move | Z: Select | X: Cancel";
    const char *hintLine = "Hint: The maze entrance is located at the bottom-left corner.";
    
    int fontSize = 22; 

    int lineWidth1 = MeasureText(line1, fontSize);
    int lineWidth2 = MeasureText(line2, fontSize);
    int hintWidth = MeasureText(hintLine, fontSize);

    int textX1 = (GetScreenWidth() - lineWidth1) / 2;
    int textX2 = (GetScreenWidth() - lineWidth2) / 2;
    int hintX = (GetScreenWidth() - hintWidth) / 2;

    DrawText(line1, textX1, startY + totalSize + 30, fontSize, LIGHTGRAY);
    DrawText(line2, textX2, startY + totalSize + 30 + 25, fontSize, WHITE);
    DrawText(hintLine, hintX, startY + totalSize + 30 + 50, fontSize, YELLOW); 
}

// ---------------------------
// 繪製對話框
// ---------------------------
void DrawDialogue2(const GameState *state)
{
    DrawRectangle(150, 700, 980, 180, Fade(BLACK, 0.85f));
    DrawRectangleLines(150, 700, 980, 180, WHITE);

    if (dialogueLines != NULL && dialogueIndex < dialogueCount) {
        DrawTextEx(state->storyFont, dialogueLines[dialogueIndex], (Vector2){200, 730}, 32, 1, WHITE);
    }
    DrawText("[Press Z to Continue]", 850, 820, 20, GRAY);
}

// ---------------------------
// 绘制完成地图
// ---------------------------
void DrawCompletedMap(const GameState *state)
{
    int totalSize = 3 * PUZZLE_TILE_SIZE; 
    int startX = (GetScreenWidth() - totalSize) / 2;
    int startY = (GetScreenHeight() - totalSize) / 2 - 40; 

    Rectangle mapDestRec = { (float)startX, (float)startY, (float)totalSize, (float)totalSize };
    Rectangle mapSrcRec = { 0.0f, 0.0f, (float)state->mapSprite.width, (float)state->mapSprite.height };

    DrawTexturePro(state->mapSprite, mapSrcRec, mapDestRec, (Vector2){0, 0}, 0.0f, WHITE);
    DrawRectangleLinesEx((Rectangle){mapDestRec.x - 5, mapDestRec.y - 5, mapDestRec.width + 10, mapDestRec.height + 10}, 4.0f, DARKGRAY);
}

// ---------------------------
// 绘制 Level2
// ---------------------------
void DrawLevel2(const GameState *state)
{
    DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Fade(BLACK, 0.1f));

    if (!showPuzzle) 
    {
        // 頂層吊燈微弱動態漫射光效果 (Glow)
        DrawCircleGradient((Vector2){640, 40}, 350, Fade(YELLOW, 0.08f), Fade(BLACK, 0.0f));
        DrawCircleGradient((Vector2){640, 40}, 150, Fade(WHITE, 0.12f), Fade(BLACK, 0.0f));

        // 💡 核心修改：利用迴圈將 20 個獨立字元以旋轉角度（DrawTextPro）無底色、低不透明度棕色畫在牆上
        Color wallCodeColor = Fade(BROWN, 0.28f); // 穩定的低透明度棕色
        
        // 安全起見，如果因意外未初始化，則確保有基礎數據
        if (isScatteredTextInitialized) {
            for (int i = 0; i < 20; i++) {
                // Raylib 的 DrawTextPro 支援文字旋轉，需要傳入：字型、文字、座標、旋轉中心點、旋轉角度、大小、間距、顏色
                // 我們將旋轉中心點設為 (0,0)，並把隨機生成的角度代入
                Vector2 origin = { 0, 0 };
                DrawTextPro(
                    state->storyFont, 
                    wallTextList[i].character, 
                    wallTextList[i].pos, 
                    origin, 
                    wallTextList[i].rotation, 
                    wallTextList[i].scale, 
                    2, 
                    wallCodeColor
                );
            }
        }

        // 大落體書堆（paperPileSprite）的巨型沉降陰影
        DrawEllipse(690, 875, 140, 25, Fade(BLACK, 0.65f)); 

        // 渲染 paperPile.png 貼圖
        Rectangle paperSrcRec = { 0.0f, 0.0f, (float)state->paperPileSprite.width, (float)state->paperPileSprite.height };
        DrawTexturePro(state->paperPileSprite, paperSrcRec, paperPile, (Vector2){0, 0}, 0.0f, Fade(WHITE, 0.94f));

        // 材料複用干擾術 (Asset Flip)
        DrawEllipse(230, 715, 45, 10, Fade(BLACK, 0.4f)); 
        Rectangle bDest1 = { 150, 650, 150, 75 };
        Rectangle bSrc = { 0.0f, 0.0f, (float)state->bookSprite.width, (float)state->bookSprite.height };
        DrawTexturePro(state->bookSprite, bSrc, bDest1, (Vector2){0, 0}, 0.0f, Fade(GRAY, 0.5f));

        // 右下角散落的泛黃廢紙
        DrawRectanglePro((Rectangle){980, 770, 35, 45}, (Vector2){0,0}, 15.0f, Fade(LIGHTGRAY, 0.4f));
        DrawRectanglePro((Rectangle){1030, 790, 40, 30}, (Vector2){0,0}, -25.0f, Fade(GRAY, 0.35f));

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

    if (showPuzzle){
        DrawPuzzle(state); 
    }

    if (showMap){
        DrawCompletedMap(state); 
    }

    if (showDialogue){
        DrawDialogue2(state);
    }
}
