#!/bin/bash

# 確保 build 資料夾存在
mkdir -p build

# 執行 GCC 編譯
gcc src/*.c -o build/game_exe -Iinclude -lraylib -lGL -lm -lpthread -ldl -lrt -lX11 -lSDL2 -lSDL2_mixer

if [ $? -eq 0 ]; then
    echo "編譯成功！啟動遊戲..."
    ./build/game_exe
else
    echo "編譯失敗！"
fi
