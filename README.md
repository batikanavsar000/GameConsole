🎮 ESP32 Retro Gaming Console (Pacman & Flappy Friends)

An ESP32-powered hand-held retro gaming console featuring a unified custom bootloader interface, an interactive game selection menu, and two fully functional games: classic **Pacman** and a highly personalized version of Flappy Bird called **Flappy Friends**.

This project integrates custom SPI hardware drivers, matrix keypad scanning, and a custom rendering engine that loads 21 hand-drawn pixel art character frames directly from an SD card.

---

 Key Features

*   **Console Bootloader & OS Style Menu:** A centralized entry state allowing users to navigate and launch different games seamlessly using the 4x4 matrix keypad.
*   **7 Playable Characters (Flappy Friends):** Features 7 custom characters designed after real-life friends (**İrem, Han, BigBoy, Batıkan, Azra, Ayça, Arda**).
*   **Custom LibreSprite Pixel Art:** Each of the 7 characters contains 3 custom-animated behavior frames (Normal, Flapping, Scared/Dead) drawn entirely from scratch in LibreSprite (Totaling 21 unique 32x32 pixel 16-bit BMP structures).
*   **Fake Transparency Rendering Engine:** Features a direct `GFXcanvas16` buffer manipulation routine (`drawSpriteWithBg`) that overrides pure white background pixels (`0xFFFF`) to inject dynamic color backgrounds, preventing solid square artifact boxes around the sprites.
*   **Chiptune Chords & Sound FX:** Immersive 8-bit audio feedback synced with jumps, scores, selection changes, and game-over sequences using a passive buzzer.
*   **Adaptive Difficulty:** Real-time variable adjustment changing physics speed parameters dynamically as the game progression updates (Levels 1 to 3).

---

## 🛠️ Hardware Architecture

| Component | Specifications / Connection Pins |
| --- | --- |
| **Microcontroller** | ESP32 (DevKit V1 NodeMCU) |
| **Display Panel** | 1.8" ST7735 TFT Color LCD (SPI communication, BGR panel color mapping) |
| **Input Matrix** | 4x4 Membrane Matrix Keypad |
| **Storage Unit** | Micro SD Card SPI Module (Stores game assets and sprite arrays) |


📌 Schematic & Pin Configurations
*   **TFT Display Subsystem:** `CS` ➡️ GPIO 15, `DC` ➡️ GPIO 2, `RST` ➡️ GPIO 4
*   **SD Storage Subsystem:** `CS` ➡️ GPIO 5
*   **Keypad Rows:** GPIO 25, 26, 27, 32
*   **Keypad Columns:** GPIO 22, 21, 14, 13

---

📂 SD Card File Hierarchy

Format your micro SD card to **FAT16/FAT32**. Create an `images` root directory and place the 32x32 pixel 16-bit BMP sprite files inside using the exact naming structure below:

```text
SD Card Root
└── 📁 images
    ├── 📄 h1_r1.bmp, h1_r2.bmp, h1_r3.bmp  (İrem - 3 animation frames)
    ├── 📄 h2_r1.bmp, h2_r2.bmp, h2_r3.bmp  (Han - 3 animation frames)
    ├── 📄 h3_r1.bmp, h3_r2.bmp, h3_r3.bmp  (BigBoy - 3 animation frames)
    ├── 📄 h4_r1.bmp, h4_r2.bmp, h4_r3.bmp  (Batıkan - 3 animation frames)
    ├── 📄 h5_r1.bmp, h5_r2.bmp, h5_r3.bmp  (Azra - 3 animation frames)
    ├── 📄 h6_r1.bmp, h6_r2.bmp, h6_r3.bmp  (Ayça - 3 animation frames)
    └── 📄 h7_r1.bmp, h7_r2.bmp, h7_r3.bmp  (Arda - 3 animation frames)
