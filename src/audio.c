#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "audio.h" 

// 確保頂部有這個紀錄當前播放 BGM 索引的變數
static int current_playing_level = -1;

// 宣告指標儲存音樂與音效
Mix_Music *bgms[5] = {NULL};
Mix_Chunk *sound_win = NULL;
Mix_Chunk *sound_fail = NULL;
Mix_Chunk *sound_alarm = NULL;
Mix_Chunk *sound_get_tool = NULL;
Mix_Chunk *sound_success = NULL;

int init_audio() {
    // 💡 1. 初始化 SDL 的音訊子系統
    if (SDL_Init(SDL_INIT_AUDIO) < 0) {
        return 0;
    }

    // 💡 2. 開啟音訊設備（設定 44100Hz 雙聲道、16位元）
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) {
        return 0;
    }

    // 💡 3. 顯式初始化 MP3 解碼器
    int flags = MIX_INIT_MP3;
    if ((Mix_Init(flags) & flags) != flags) {
        // 解碼器初始化失敗
    }

    // 💡 4. 分配足夠的音效播放通道
    Mix_AllocateChannels(8);

    // ---- 素材載入 ----
    bgms[0] = Mix_LoadMUS("assets/audio/bgm0.mp3");
    bgms[1] = Mix_LoadMUS("assets/audio/bgm1.mp3");
    bgms[2] = Mix_LoadMUS("assets/audio/bgm2.mp3");
    bgms[3] = Mix_LoadMUS("assets/audio/bgm3.mp3");
    bgms[4] = Mix_LoadMUS("assets/audio/bgm4.mp3");

    // 💡 防盲點除錯偵測：如果路徑不對或檔案損壞，終端機會在啟動時噴印出來
    if (bgms[4] == NULL) {
        printf("❌ [SDL_mixer 錯誤] 找不到或無法解析 assets/audio/bgm4.mp3! 原因: %s\n", Mix_GetError());
    }

    // 載入短音效
    sound_win   = Mix_LoadWAV("assets/audio/win.wav");
    sound_fail  = Mix_LoadWAV("assets/audio/fail.wav");
    sound_alarm = Mix_LoadWAV("assets/audio/alarm.wav");
    sound_get_tool = Mix_LoadWAV("assets/audio/get_tool.wav");
    sound_success  = Mix_LoadWAV("assets/audio/success.wav");

    // 💡 5. 一開遊戲就讓主畫面音樂優雅淡入 1.5 秒
    if (bgms[0] != NULL) {
        Mix_FadeInMusic(bgms[0], -1, 1500); 
        current_playing_level = 0;
    }

    return 1;
}

void play_bgm(int level) {
    if (level >= 0 && level < 5 && bgms[level] != NULL) {
        // 💡 防護機制：如果已經在播這關的歌，就直接 return 不重複觸發
        if (level == current_playing_level && Mix_PlayingMusic()) {
            return; 
        }

        int fade_time_ms = 1500; 

        // 直接對當前播放的音樂軌道下達強制的 Mix_HaltMusic() 截斷命令清空通道，
        // 這樣可以 100% 確保新音樂（特別是最後的 bgm4）在淡入時不會被前一首殘留的緩衝區卡死。
        if (Mix_PlayingMusic()) {
            Mix_HaltMusic(); 
        }

        // 完美安全淡入
        if (Mix_FadeInMusic(bgms[level], -1, fade_time_ms) < 0) {
            printf("❌ 背景音樂播放失敗: %s\n", Mix_GetError());
        }

        current_playing_level = level; // 更新當前播放紀錄
    }
}

void stop_bgm() {
    // 停止音樂時，在 1 秒內淡出
    Mix_FadeOutMusic(1000);
    current_playing_level = -1; 
}

void play_effect_win() {
    if (sound_win) Mix_PlayChannel(-1, sound_win, 0);
}

void play_effect_fail() {
    if (sound_fail) Mix_PlayChannel(-1, sound_fail, 0);
}

void play_effect_alarm() {
    if (sound_alarm) Mix_PlayChannel(-1, sound_alarm, 0);
}

void play_effect_get_tool() {
    if (sound_get_tool) Mix_PlayChannel(-1, sound_get_tool, 0);
}

void play_effect_success() { 
    if (sound_success) Mix_PlayChannel(-1, sound_success, 0); 
}

void close_audio() {
    for (int i = 0; i < 5; i++) {
        if (bgms[i]) Mix_FreeMusic(bgms[i]);
    }
    if (sound_win) Mix_FreeChunk(sound_win);
    if (sound_fail) Mix_FreeChunk(sound_fail);
    if (sound_alarm) Mix_FreeChunk(sound_alarm);
    if (sound_get_tool) Mix_FreeChunk(sound_get_tool);
    if (sound_success) Mix_FreeChunk(sound_success);

    Mix_CloseAudio();
}