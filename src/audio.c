#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include "audio.h" 

// 宣告指標儲存音樂與音效
Mix_Music *bgms[4] = {NULL};
Mix_Chunk *sound_win = NULL;
Mix_Chunk *sound_fail = NULL;
Mix_Chunk *sound_alarm = NULL;
Mix_Chunk *sound_get_tool = NULL;

int init_audio() {
    if (SDL_Init(SDL_INIT_AUDIO) < 0) return 0;
    if (Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 2048) < 0) return 0;

    // 載入 4 個畫面的背景音樂
    bgms[0] = Mix_LoadMUS("assets/audio/bgm0.mp3");
    bgms[1] = Mix_LoadMUS("assets/audio/bgm1.mp3");
    bgms[2] = Mix_LoadMUS("assets/audio/bgm2.mp3");
    bgms[3] = Mix_LoadMUS("assets/audio/bgm3.mp3");

    // 載入短音效
    sound_win = Mix_LoadWAV("assets/audio/win.wav");
    sound_fail = Mix_LoadWAV("assets/audio/fail.wav");
    sound_alarm = Mix_LoadWAV("assets/audio/alarm.wav");
    sound_get_tool = Mix_LoadWAV("assets/audio/get_tool.wav");

    return 1;
}

void play_bgm(int level) {
    if (level >= 0 && level < 4 && bgms[level] != NULL) {
        Mix_PlayMusic(bgms[level], -1); // -1 代表無限循環
    }
}

void stop_bgm() {
    Mix_HaltMusic();
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

void close_audio() {
    for (int i = 0; i < 4; i++) {
        if (bgms[i]) Mix_FreeMusic(bgms[i]);
    }
    if (sound_win) Mix_FreeChunk(sound_win);
    if (sound_fail) Mix_FreeChunk(sound_fail);
    if (sound_alarm) Mix_FreeChunk(sound_alarm);
    if (sound_get_tool) Mix_FreeChunk(sound_get_tool);

    Mix_CloseAudio();
}