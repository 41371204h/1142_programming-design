#ifndef LEVEL1_H
#define LEVEL1_H

#include "game_shared.h"

// initialize level 1 variables
void InitLevel1(void);

// Update level 1 logic: receive state from main_hub and conduct item/status modification
void UpdateLevel1(GameState *state);

// 繪製第一關畫面 (接收大腦傳來的 state 讀取 Q 版人物貼圖)
void DrawLevel1(const GameState *state);

void DrawPaperText(void);
void DrawMorseTable(const GameState *state);
void DrawNavigationCommand(void);

#endif // LEVEL1_H
