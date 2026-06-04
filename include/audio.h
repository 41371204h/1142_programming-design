#ifndef AUDIO_H
#define AUDIO_H

// 初始化與關閉
int init_audio();
void close_audio();

// 背景音樂控制 (0: 主畫面, 1: 遊戲一, 2: 遊戲二, 3: 遊戲三)
void play_bgm(int level);
void stop_bgm();

// 音效控制
void play_effect_win();     // 通關
void play_effect_fail();    // 失敗
void play_effect_alarm();   // 警報
void play_effect_get_tool(); // 拿到把柄

#endif