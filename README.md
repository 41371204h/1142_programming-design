# Depth Zero
A narrative-driven **mystery puzzle game** built from scratch using the `raylib` library.

## 📖 The Story
You are a researcher at a deep-sea observation station. One day, you wake up to find that your colleagues have all disappeared, leaving you alone in the station.
You also discover that multiple systems have malfunctioned, including the critical oxygen supply system.
Oxygen is running out, but it’s too late for you to repair the entire station.

Your only chance of survival is to restart the escape system and gain access to the escape pod.
To launch the escape pod, you must first **repair three major systems** to obtain launch authorization. Can you escape successfully before the oxygen runs out?

## 🎮 Key Features
* 3 puzzle levels players must clear to escape a damaged space facility
* Inventory system to collect clues and items
* Secret sequence mechanic for level 3 unlock
* Dialogue & storytelling with character progression
* Sprite-based graphics using raylib
* Audio system with background music

## 🛠️ Tech Stack & Requirements
* **Language**: [C]
* **Graphics Library**: [raylib](https://raylib.com)
* **Compiler**: GNU C Compiler (GCC)
* **Build System**: Standalone Shell Script (`build.sh`)

## 📦 How to Build and Run
We have included a convenient shell script (`build.sh`) that automates the GCC compilation and linking process for raylib.

### Prerequisites
* **Tested OS**: Ubuntu 24.04
* Make sure you have the **GNU C Compiler (GCC)**, **raylib** and the **development package for SDL2/SDL_mixer** installed on your system.

#### Install Raylib on Ubuntu
[Working on GNU Linux](https://github.com/raysan5/raylib/wiki/Working-on-GNU-Linux)
#### Installing SDL2/SDL
```bash
sudo apt update && sudo apt install libsdl2-dev libsdl2-mixer-dev -y
```

### Steps
1. Clone the repository and navigate into the folder:
   ```bash
   git clone https://github.com/41371204h/1142_programming-design
   cd 1142_programming-design
   ```
2. Give execution permission to the script (if needed):
   ```bash
   chmod +x build.sh
   ```

3. Build and run the game instantly:
   ```bash
   ./build.sh
   ```

## 📁 Project Structure
```
1142_programming-design/
├── .gitignore          # 守門員：阻擋執行檔等垃圾上傳
├── build.sh            # 編譯工具：Ubuntu 一鍵編譯腳本
├── assets/             # 資源庫：存放圖片、音效
│
├── include/            # 契約庫：存放所有的 .h 標頭檔
│   └── audio.h
│   └── game_shared.h   # 核心地圖：遊戲全域狀態與進度結構
│   └── level1.h
│   └── level2.h
│   └── level3.h
└── src/                # 邏輯庫：存放所有的 .c 程式碼
    └── audio.c         # 處理遊戲音效的程式
    └── level1.c        # 繪製與控制第一關的程式
    └── level2.c        # 繪製與控制第二關的程式
    └── level3.c        # 繪製與控制第三關的程式
    └── main_hub.c      # 主程式：遊戲迴圈與場景切換
```
