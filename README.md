# 3 Door Puzzle Game
## Description
## Structure
```
1142_programming-design/
├── .gitignore          # 守門員：阻擋執行檔等垃圾上傳
├── build.sh            # 編譯工具：Ubuntu 一鍵編譯腳本
├── assets/             # 資源庫：存放圖片、音效
│
├── include/            # 契約庫：存放所有的 .h 標頭檔
│   └── game_shared.h   # 核心地圖：遊戲全域狀態與進度結構
│   └── level1.h
│   └── level2.h
│   └── level3.h
└── src/                # 邏輯庫：存放所有的 .c 程式碼
    └── main_hub.c      # 主程式：遊戲迴圈與場景切換
    └── level1.c        # 繪製與控制第一關的程式
    └── level2.c        # 繪製與控制第二關的程式
    └── level3.c        # 繪製與控制第三關的程式
```
## Installation
## Usage
