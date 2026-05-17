// =============================================================
//  BATIKAN GAME CONSOLE - All-in-one Sketch
//  Launcher + Pac-Man + Flappy Friends in a single firmware
//  Each game has its own intro/welcome screen
//  Hardware: ESP32 + 1.8" ST7735 TFT + 4x4 Keypad + SD Card
// =============================================================
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Adafruit_ImageReader.h>
#include <SdFat.h>
#include <SPI.h>
#include <Keypad.h>

// ----- Forward declarations for Arduino auto-prototype generator -----
struct MenuItem;
struct PMGhost;
struct FLPipe;

// ===== Hardware Pins =====
#define SD_CS      5
#define TFT_CS     15
#define TFT_DC      2
#define TFT_RST     4
#define BUZZER_PIN 17

// ===== TFT + SD =====
SdFat                SD;
Adafruit_ImageReader reader(SD);
Adafruit_ST7735      tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);
bool sdReady = false;

// ===== Colors (BGR panel) =====
#define COL_BLACK     0x0000
#define COL_WHITE     0xFFFF
#define COL_YELLOW    0x07FF
#define COL_BLUE      0xF800
#define COL_RED       0x001F
#define COL_GREEN     0x07E0
#define COL_DARKGREEN 0x0420
#define COL_CYAN      0xFFE0
#define COL_ORANGE    0x021F
#define COL_PURPLE    0xF81F
#define COL_PINK      0x041F
#define COL_GRAY      0x528A
#define COL_DARKGRAY  0x2104
#define COL_HILIGHT   0x39E7
#define COL_DARKBLUE  0x7800
#define COL_SKY       0xFBEF
#define COL_GROUND    0x041F
#define COL_GRASS     0x0420

// ===== Keypad =====
const byte ROWS = 4;
const byte COLS = 4;
char keys[ROWS][COLS] = {
  {'1','2','3','A'},
  {'4','5','6','B'},
  {'7','8','9','C'},
  {'*','0','#','D'}
};
byte rowPins[ROWS] = {25, 26, 27, 32};
byte colPins[COLS] = {22, 21, 14, 13};
Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

// ===== Sound helper =====
const bool ENABLE_SOUND = true;
void playTone(int freq, int dur) {
  if (ENABLE_SOUND) tone(BUZZER_PIN, freq, dur);
}

// ===== Global Application State =====
enum AppState {
  S_BOOT, S_MAIN, S_MYGAMES, S_ROMS, S_SETTINGS, S_ABOUT, S_LAUNCH_ROM,
  S_PACMAN, S_FLAPPY
};
AppState appState = S_BOOT;
unsigned long stateStart = 0;

void enterState(AppState s);
bool isSelectKey(char k);
bool isBackKey(char k);

// =============================================================
//                     LAUNCHER MENU DATA
// =============================================================
struct MenuItem {
  const char* label;
  uint16_t    color;
};

const int MAIN_COUNT = 3;
MenuItem mainMenu[MAIN_COUNT] = {
  {"MY GAMES",     COL_YELLOW},
  {"GAMES (ROMs)", COL_CYAN},
  {"SETTINGS",     COL_GREEN}
};

const int MYGAMES_COUNT = 2;
MenuItem myGames[MYGAMES_COUNT] = {
  {"PAC-MAN",        COL_YELLOW},
  {"FLAPPY FRIENDS", COL_ORANGE}
};

const int ROMS_COUNT = 3;
MenuItem romsList[ROMS_COUNT] = {
  {"Super Mario Land",     COL_RED},
  {"Amazing Spider-Man",   COL_BLUE},
  {"Bugs Bunny Castle II", COL_PURPLE}
};

const int SETTINGS_COUNT = 3;
MenuItem settingsList[SETTINGS_COUNT] = {
  {"Sound On/Off", COL_CYAN},
  {"Brightness",   COL_YELLOW},
  {"About",        COL_GREEN}
};

int mainSel = 0;
int subSel  = 0;
const char* launchingName = "";

// =============================================================
//                       SETUP & MAIN LOOP
// =============================================================
void setup() {
  Serial.begin(115200);
  delay(150);
  Serial.println("\n=== BATIKAN GAME CONSOLE booting ===");

  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, LOW);

  tft.initR(INITR_GREENTAB);
  tft.setRotation(1);
  tft.fillScreen(COL_BLACK);
  Serial.println("TFT initialized");

  // SD card (needed by Flappy Friends sprites)
  if (SD.begin(SD_CS, SD_SCK_MHZ(10))) {
    sdReady = true;
    Serial.println("SD card OK");
  } else {
    sdReady = false;
    Serial.println("WARNING: SD card not found - Flappy sprites will show as ?");
  }

  randomSeed(analogRead(34));
  enterState(S_BOOT);
}

void loop() {
  switch (appState) {
    case S_BOOT:        boot_handle();       break;
    case S_MAIN:        main_handle();       break;
    case S_MYGAMES:     mygames_handle();    break;
    case S_ROMS:        roms_handle();       break;
    case S_SETTINGS:    settings_handle();   break;
    case S_ABOUT:       about_handle();      break;
    case S_LAUNCH_ROM:  launchrom_handle();  break;
    case S_PACMAN:      pacman_loop();       break;
    case S_FLAPPY:      flappy_loop();       break;
  }
}

void enterState(AppState s) {
  appState = s;
  stateStart = millis();
  switch (s) {
    case S_BOOT:        boot_draw();        break;
    case S_MAIN:        main_draw();        break;
    case S_MYGAMES:     mygames_draw();     break;
    case S_ROMS:        roms_draw();        break;
    case S_SETTINGS:    settings_draw();    break;
    case S_ABOUT:       about_draw();       break;
    case S_LAUNCH_ROM:  launchrom_draw();   break;
    case S_PACMAN:      pacman_enter();     break;
    case S_FLAPPY:      flappy_enter();     break;
  }
}

bool isSelectKey(char k) {
  return k == '5' || k == '1' || k == '3' || k == 'A' || k == 'B' || k == 'D';
}
bool isBackKey(char k) {
  return k == '*' || k == '#' || k == 'C';
}

// =============================================================
//                          BOOT INTRO
// =============================================================
void boot_draw() {
  tft.fillScreen(COL_BLACK);
  tft.setTextSize(3);
  tft.setTextColor(COL_RED);
  tft.setCursor(11, 27);
  tft.print("BATIKAN");
  tft.setTextColor(COL_YELLOW);
  tft.setCursor(8, 24);
  tft.print("BATIKAN");

  tft.setTextSize(1);
  tft.setTextColor(COL_CYAN);
  tft.setCursor(34, 60);
  tft.print("GAME CONSOLE");

  tft.drawFastHLine(20, 75, 120, COL_GREEN);
  tft.drawFastHLine(20, 76, 120, COL_DARKGREEN);

  tft.fillCircle(30, 92, 3, COL_YELLOW);
  tft.fillCircle(80, 92, 3, COL_ORANGE);
  tft.fillCircle(130, 92, 3, COL_RED);

  tft.setTextColor(COL_WHITE);
  tft.setCursor(45, 102);
  tft.print("Loading...");

  tft.drawRect(14, 115, 132, 8, COL_GRAY);
}

void boot_handle() {
  unsigned long elapsed = millis() - stateStart;
  int progress = map(min((int)elapsed, 2500), 0, 2500, 0, 130);
  tft.fillRect(15, 116, progress, 6, COL_YELLOW);
  if (elapsed > 2800) enterState(S_MAIN);
}

// =============================================================
//                          MAIN MENU
// =============================================================
void main_draw() {
  tft.fillScreen(COL_BLACK);
  drawHeader("MAIN MENU");
  drawMenu(mainMenu, MAIN_COUNT, mainSel);
  drawFooter("S5/S7=Nav  S1=Select");
}

void main_handle() {
  char k = keypad.getKey();
  if (!k) return;

  if (k == '2' || k == '4') {
    mainSel = (mainSel - 1 + MAIN_COUNT) % MAIN_COUNT;
    drawMenu(mainMenu, MAIN_COUNT, mainSel);
    playTone(1000, 30);
  } else if (k == '8' || k == '6') {
    mainSel = (mainSel + 1) % MAIN_COUNT;
    drawMenu(mainMenu, MAIN_COUNT, mainSel);
    playTone(1000, 30);
  } else if (isSelectKey(k)) {
    playTone(1500, 60);
    subSel = 0;
    if      (mainSel == 0) enterState(S_MYGAMES);
    else if (mainSel == 1) enterState(S_ROMS);
    else if (mainSel == 2) enterState(S_SETTINGS);
  }
}

// =============================================================
//                      MY GAMES SUBMENU
// =============================================================
void mygames_draw() {
  tft.fillScreen(COL_BLACK);
  drawHeader("MY GAMES");
  drawMenu(myGames, MYGAMES_COUNT, subSel);
  drawFooter("S1=Play  *=Back");
}

void mygames_handle() {
  char k = keypad.getKey();
  if (!k) return;

  if (k == '2' || k == '4') {
    subSel = (subSel - 1 + MYGAMES_COUNT) % MYGAMES_COUNT;
    drawMenu(myGames, MYGAMES_COUNT, subSel);
  } else if (k == '8' || k == '6') {
    subSel = (subSel + 1) % MYGAMES_COUNT;
    drawMenu(myGames, MYGAMES_COUNT, subSel);
  } else if (isSelectKey(k)) {
    playTone(1500, 60);
    if (subSel == 0) enterState(S_PACMAN);
    else if (subSel == 1) enterState(S_FLAPPY);
  } else if (isBackKey(k)) {
    enterState(S_MAIN);
  }
}

// =============================================================
//                      ROMS SUBMENU
// =============================================================
void roms_draw() {
  tft.fillScreen(COL_BLACK);
  drawHeader("GAMES (ROMs)");
  drawMenu(romsList, ROMS_COUNT, subSel);
  tft.setTextSize(1);
  tft.setTextColor(COL_ORANGE);
  tft.setCursor(15, 108);
  tft.print("Emulator coming soon");
  drawFooter("*=Back");
}

void roms_handle() {
  char k = keypad.getKey();
  if (!k) return;
  if (k == '2' || k == '4') {
    subSel = (subSel - 1 + ROMS_COUNT) % ROMS_COUNT;
    roms_draw();
  } else if (k == '8' || k == '6') {
    subSel = (subSel + 1) % ROMS_COUNT;
    roms_draw();
  } else if (isSelectKey(k)) {
    launchingName = romsList[subSel].label;
    enterState(S_LAUNCH_ROM);
  } else if (isBackKey(k)) {
    enterState(S_MAIN);
  }
}

void launchrom_draw() {
  tft.fillScreen(COL_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(COL_RED);
  tft.setCursor(20, 15);
  tft.print("ROM NOT YET");
  tft.setCursor(28, 35);
  tft.print("PLAYABLE");
  tft.setTextSize(1);
  tft.setTextColor(COL_YELLOW);
  tft.setCursor(10, 60);
  tft.print(launchingName);
  tft.setTextColor(COL_WHITE);
  tft.setCursor(5, 78);
  tft.print(".gb files need a");
  tft.setCursor(5, 90);
  tft.print("Game Boy emulator");
  tft.setCursor(5, 102);
  tft.print("(coming later).");
  tft.setTextColor(COL_GREEN);
  tft.setCursor(20, 117);
  tft.print("Press any key");
}

void launchrom_handle() {
  if (keypad.getKey()) enterState(S_ROMS);
}

// =============================================================
//                       SETTINGS
// =============================================================
void settings_draw() {
  tft.fillScreen(COL_BLACK);
  drawHeader("SETTINGS");
  drawMenu(settingsList, SETTINGS_COUNT, subSel);
  drawFooter("S1=Select  *=Back");
}

void settings_handle() {
  char k = keypad.getKey();
  if (!k) return;
  if (k == '2' || k == '4') {
    subSel = (subSel - 1 + SETTINGS_COUNT) % SETTINGS_COUNT;
    drawMenu(settingsList, SETTINGS_COUNT, subSel);
  } else if (k == '8' || k == '6') {
    subSel = (subSel + 1) % SETTINGS_COUNT;
    drawMenu(settingsList, SETTINGS_COUNT, subSel);
  } else if (isSelectKey(k)) {
    if (subSel == 2) enterState(S_ABOUT);
  } else if (isBackKey(k)) {
    enterState(S_MAIN);
  }
}

void about_draw() {
  tft.fillScreen(COL_BLACK);
  drawHeader("ABOUT");
  tft.setTextSize(1);
  tft.setTextColor(COL_YELLOW);
  tft.setCursor(10, 22);
  tft.print("Batikan Game Console");
  tft.setTextColor(COL_WHITE);
  tft.setCursor(10, 38);
  tft.print("ESP32 + 1.8\" TFT");
  tft.setCursor(10, 50);
  tft.print("4x4 Keypad + SD");
  tft.setTextColor(COL_CYAN);
  tft.setCursor(10, 70);
  tft.print("Games included:");
  tft.setTextColor(COL_GREEN);
  tft.setCursor(10, 82);
  tft.print("- Pac-Man");
  tft.setCursor(10, 94);
  tft.print("- Flappy Friends");
  tft.setTextColor(COL_ORANGE);
  tft.setCursor(10, 108);
  tft.print("Made by Batikan 2026");
  drawFooter("*=Back");
}

void about_handle() {
  char k = keypad.getKey();
  if (k && (isBackKey(k) || isSelectKey(k))) enterState(S_SETTINGS);
}

// =============================================================
//                       UI HELPERS
// =============================================================
void drawHeader(const char* title) {
  tft.fillRect(0, 0, 160, 14, COL_DARKGRAY);
  tft.drawFastHLine(0, 14, 160, COL_YELLOW);
  tft.setTextSize(1);
  tft.setTextColor(COL_YELLOW);
  int len = strlen(title);
  int textX = 80 - (len * 3);
  tft.setCursor(textX, 4);
  tft.print(title);
}

void drawFooter(const char* hint) {
  tft.fillRect(0, 119, 160, 9, COL_DARKGRAY);
  tft.drawFastHLine(0, 118, 160, COL_GRAY);
  tft.setTextSize(1);
  tft.setTextColor(COL_WHITE);
  int len = strlen(hint);
  int textX = 80 - (len * 3);
  tft.setCursor(textX, 120);
  tft.print(hint);
}

void drawMenu(MenuItem* items, int count, int selected) {
  tft.fillRect(0, 18, 160, 100, COL_BLACK);
  int yStep = 22;
  int totalH = count * yStep;
  int startY = 28;
  if (totalH < 90) startY = 28 + (90 - totalH) / 2;
  for (int i = 0; i < count; i++) {
    int y = startY + i * yStep;
    if (i == selected) {
      tft.fillRoundRect(5, y - 4, 150, 18, 4, COL_HILIGHT);
      tft.fillTriangle(10, y + 4, 18, y - 1, 18, y + 9, items[i].color);
    }
    tft.setTextSize(1);
    tft.setTextColor(items[i].color);
    tft.setCursor(24, y + 2);
    tft.print(items[i].label);
  }
}


// #############################################################
// #                     PAC-MAN GAME                          #
// #############################################################

const int  PM_CELL_SIZE = 8;
const int  PM_GRID_COLS = 20;
const int  PM_GRID_ROWS = 14;
const int  PM_MAZE_Y    = 16;

const char pm_maze[PM_GRID_ROWS][PM_GRID_COLS + 1] = {
  "####################",
  "#........##........#",
  "#o##.###.##.###.##o#",
  "#.##.###.##.###.##.#",
  "#..................#",
  "#.##.#.######.#.##.#",
  "#....#...##...#....#",
  "####.###.##.###.####",
  "####.###.##.###.####",
  "#....#...##...#....#",
  "#.##.#.######.#.##.#",
  "#..................#",
  "#o.......##.......o#",
  "####################"
};

bool pm_pellets[PM_GRID_ROWS][PM_GRID_COLS];
bool pm_power[PM_GRID_ROWS][PM_GRID_COLS];
int  pm_totalPellets = 0;

int  pm_row, pm_col;
int  pm_x, pm_y;
int  pm_targetX, pm_targetY;
char pm_dir = 'L';
char pm_queuedDir = 0;
bool pm_moving = false;
unsigned long pm_lastMove = 0;
const int PM_PAC_SPEED = 35;
int pm_chompFrame = 0;
unsigned long pm_lastChomp = 0;
const int PM_CHOMP_MS = 130;

struct PMGhost {
  int row, col, x, y, targetX, targetY;
  char dir;
  bool moving, vulnerable;
  uint16_t color;
  int aiType;
  int spawnRow, spawnCol;
};
const int PM_NUM_GHOSTS = 3;
PMGhost pm_ghosts[PM_NUM_GHOSTS];
unsigned long pm_ghostLastMove = 0;
const int PM_GHOST_SPEED = 50;

int pm_score = 0, pm_lives = 3;
bool pm_powerMode = false;
unsigned long pm_powerStart = 0;
const unsigned long PM_POWER_DURATION = 6500;

// PAC-MAN sub-states: INTRO and WELCOME are shown when launching from menu
enum PMState { PM_INTRO, PM_WELCOME, PM_PLAYING, PM_GAMEOVER, PM_WIN };
PMState pm_state = PM_INTRO;
unsigned long pm_subStart = 0;
unsigned long pm_lastBlink = 0;
bool pm_blinkOn = true;

// ----- Entry from launcher: always start with intro -----
void pacman_enter() {
  pm_state = PM_INTRO;
  pm_subStart = millis();
  pm_lastBlink = millis();
  pm_blinkOn = true;
  pm_drawIntro();
}

void pacman_loop() {
  switch (pm_state) {
    case PM_INTRO:    pm_handleIntro();    break;
    case PM_WELCOME:  pm_handleWelcome();  break;
    case PM_PLAYING:  pacman_handlePlaying(); break;
    case PM_GAMEOVER:
    case PM_WIN:      pacman_handleEnd();  break;
  }
}

// Restart the actual game (without intro) - used by retry
void pacman_restart() {
  pm_state = PM_PLAYING;
  pm_score = 0;
  pm_lives = 3;
  pm_powerMode = false;
  pm_initMaze();
  pm_initPac();
  pm_initGhosts();
  pm_drawGameStatic();
}

// ----- Pac-Man intro screen -----
void pm_drawIntroGhost(int x, int y, uint16_t color) {
  tft.fillCircle(x + 4, y + 3, 4, color);
  tft.fillRect(x, y + 3, 9, 5, color);
  tft.drawPixel(x,     y + 8, color);
  tft.drawPixel(x + 2, y + 8, color);
  tft.drawPixel(x + 4, y + 8, color);
  tft.drawPixel(x + 6, y + 8, color);
  tft.drawPixel(x + 8, y + 8, color);
  tft.fillRect(x + 2, y + 3, 2, 2, COL_WHITE);
  tft.fillRect(x + 5, y + 3, 2, 2, COL_WHITE);
  tft.drawPixel(x + 3, y + 4, COL_BLUE);
  tft.drawPixel(x + 6, y + 4, COL_BLUE);
}

void pm_drawIntro() {
  tft.fillScreen(COL_BLACK);

  // Big yellow PAC-MAN title
  tft.setTextSize(2);
  tft.setTextColor(COL_YELLOW);
  tft.setCursor(35, 18);
  tft.print("PAC-MAN");

  // Four ghost decorations in a row
  pm_drawIntroGhost(20, 55, COL_RED);
  pm_drawIntroGhost(50, 55, COL_PINK);
  pm_drawIntroGhost(80, 55, COL_CYAN);
  pm_drawIntroGhost(110, 55, COL_ORANGE);

  // Big Pac-Man character
  int cx = 80, cy = 85;
  tft.fillCircle(cx, cy, 8, COL_YELLOW);
  tft.fillTriangle(cx, cy, cx + 10, cy - 6, cx + 10, cy + 6, COL_BLACK);
}

void pm_handleIntro() {
  // Blinking "Press any key"
  if (millis() - pm_lastBlink > 450) {
    pm_lastBlink = millis();
    pm_blinkOn = !pm_blinkOn;
    tft.setTextSize(1);
    tft.setCursor(28, 110);
    tft.setTextColor(pm_blinkOn ? COL_WHITE : COL_BLACK);
    tft.print("Press any key!");
  }

  // Minimum display time guard - prevents instant skip from buffered menu key
  if (millis() - pm_subStart < 800) {
    keypad.getKey();    // discard any pending key from menu
    return;
  }

  char k = keypad.getKey();
  if (k) {
    if (isBackKey(k)) {
      enterState(S_MYGAMES);
      return;
    }
    playTone(1500, 60);
    pm_state = PM_WELCOME;
    pm_subStart = millis();
    pm_drawWelcome();
  }
}

// ----- Welcome screen -----
void pm_drawWelcome() {
  tft.fillScreen(COL_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(COL_WHITE);
  tft.setCursor(38, 30);
  tft.print("Welcome");

  tft.setTextSize(2);
  tft.setTextColor(COL_YELLOW);
  tft.setCursor(30, 55);
  tft.print("BATIKAN");

  tft.setTextSize(1);
  tft.setTextColor(COL_CYAN);
  tft.setCursor(20, 90);
  tft.print("Game starting...");
}

void pm_handleWelcome() {
  if (millis() - pm_subStart > 2500) {
    pacman_restart();   // actually start game
  }
}

// ----- Init helpers -----
void pm_initMaze() {
  pm_totalPellets = 0;
  for (int r = 0; r < PM_GRID_ROWS; r++) {
    for (int c = 0; c < PM_GRID_COLS; c++) {
      char ch = pm_maze[r][c];
      pm_pellets[r][c] = (ch == '.');
      pm_power[r][c]   = (ch == 'o');
      if (ch == '.' || ch == 'o') pm_totalPellets++;
    }
  }
}

void pm_initPac() {
  pm_row = 11; pm_col = 10;
  pm_x = pm_col * PM_CELL_SIZE;
  pm_y = PM_MAZE_Y + pm_row * PM_CELL_SIZE;
  pm_targetX = pm_x;
  pm_targetY = pm_y;
  pm_dir = 'L';
  pm_queuedDir = 0;
  pm_moving = false;
}

void pm_initGhosts() {
  int cols[PM_NUM_GHOSTS] = {9, 10, 11};
  uint16_t colors[PM_NUM_GHOSTS] = {COL_RED, COL_PINK, COL_CYAN};
  for (int i = 0; i < PM_NUM_GHOSTS; i++) {
    pm_ghosts[i].spawnRow = 4;
    pm_ghosts[i].spawnCol = cols[i];
    pm_ghosts[i].row = 4; pm_ghosts[i].col = cols[i];
    pm_ghosts[i].x = cols[i] * PM_CELL_SIZE;
    pm_ghosts[i].y = PM_MAZE_Y + 4 * PM_CELL_SIZE;
    pm_ghosts[i].targetX = pm_ghosts[i].x;
    pm_ghosts[i].targetY = pm_ghosts[i].y;
    pm_ghosts[i].dir = 'D';
    pm_ghosts[i].moving = false;
    pm_ghosts[i].vulnerable = false;
    pm_ghosts[i].color = colors[i];
    pm_ghosts[i].aiType = i;
  }
}

void pm_drawGameStatic() {
  tft.fillScreen(COL_BLACK);
  pm_drawMaze();
  pm_drawAllPellets();
  pm_drawHUD();
  pm_drawPac();
  for (int i = 0; i < PM_NUM_GHOSTS; i++) pm_drawGhost(i);
}

// ----- Playing loop -----
void pacman_handlePlaying() {
  char k = keypad.getKey();
  if (k) {
    if (isBackKey(k)) {
      enterState(S_MYGAMES);
      return;
    }
    switch (k) {
      case '1': case '2': pm_queuedDir = 'U'; break;
      case '7': case '8': pm_queuedDir = 'D'; break;
      case '4':           pm_queuedDir = 'L'; break;
      case '6':           pm_queuedDir = 'R'; break;
    }
  }

  if (millis() - pm_lastMove >= PM_PAC_SPEED) {
    pm_lastMove = millis();
    pm_updatePac();
  }

  if (millis() - pm_ghostLastMove >= PM_GHOST_SPEED) {
    pm_ghostLastMove = millis();
    for (int i = 0; i < PM_NUM_GHOSTS; i++) pm_updateGhost(i);
  }

  if (millis() - pm_lastChomp >= PM_CHOMP_MS) {
    pm_lastChomp = millis();
    pm_chompFrame = 1 - pm_chompFrame;
    pm_erasePac();
    pm_drawPac();
  }

  if (pm_powerMode && millis() - pm_powerStart > PM_POWER_DURATION) {
    pm_powerMode = false;
    for (int i = 0; i < PM_NUM_GHOSTS; i++) {
      pm_ghosts[i].vulnerable = false;
      pm_eraseGhost(i);
      pm_drawGhost(i);
    }
  }

  pm_checkCollisions();
  if (pm_totalPellets == 0) {
    pm_state = PM_WIN;
    pm_drawWinScreen();
  }
}

void pacman_handleEnd() {
  char k = keypad.getKey();
  if (!k) return;
  if (isBackKey(k)) {
    enterState(S_MYGAMES);
  } else {
    pacman_restart();
  }
}

// ----- Movement -----
void pm_updatePac() {
  if (pm_moving) {
    pm_erasePac();
    if (pm_x < pm_targetX) pm_x++;
    else if (pm_x > pm_targetX) pm_x--;
    if (pm_y < pm_targetY) pm_y++;
    else if (pm_y > pm_targetY) pm_y--;
    pm_drawPac();
    if (pm_x == pm_targetX && pm_y == pm_targetY) {
      pm_moving = false;
      pm_eatPellet();
    }
  } else {
    char tryDir = (pm_queuedDir != 0 && pm_canStep(pm_row, pm_col, pm_queuedDir))
                  ? pm_queuedDir : pm_dir;
    if (pm_canStep(pm_row, pm_col, tryDir)) {
      pm_dir = tryDir;
      if (tryDir == pm_queuedDir) pm_queuedDir = 0;
      pm_startStep();
    }
  }
}

void pm_eatPellet() {
  if (pm_pellets[pm_row][pm_col]) {
    pm_pellets[pm_row][pm_col] = false;
    pm_totalPellets--;
    pm_score += 10;
    pm_drawHUD();
    playTone(1200, 20);
  }
  if (pm_power[pm_row][pm_col]) {
    pm_power[pm_row][pm_col] = false;
    pm_totalPellets--;
    pm_score += 50;
    pm_powerMode = true;
    pm_powerStart = millis();
    for (int i = 0; i < PM_NUM_GHOSTS; i++) {
      pm_ghosts[i].vulnerable = true;
      pm_eraseGhost(i);
      pm_drawGhost(i);
    }
    pm_drawHUD();
    playTone(1800, 100);
  }
}

void pm_startStep() {
  int nr = pm_row, nc = pm_col;
  switch (pm_dir) {
    case 'U': nr--; break; case 'D': nr++; break;
    case 'L': nc--; break; case 'R': nc++; break;
  }
  if (!pm_inBounds(nr, nc) || pm_isWall(nr, nc)) return;
  pm_row = nr; pm_col = nc;
  pm_targetX = pm_col * PM_CELL_SIZE;
  pm_targetY = PM_MAZE_Y + pm_row * PM_CELL_SIZE;
  pm_moving = true;
}

bool pm_canStep(int r, int c, char dir) {
  int nr = r, nc = c;
  switch (dir) {
    case 'U': nr--; break; case 'D': nr++; break;
    case 'L': nc--; break; case 'R': nc++; break;
  }
  return pm_inBounds(nr, nc) && !pm_isWall(nr, nc);
}

bool pm_inBounds(int r, int c) {
  return r >= 0 && r < PM_GRID_ROWS && c >= 0 && c < PM_GRID_COLS;
}

bool pm_isWall(int r, int c) {
  if (!pm_inBounds(r, c)) return true;
  return pm_maze[r][c] == '#';
}

// ----- Ghost AI -----
void pm_updateGhost(int idx) {
  PMGhost& g = pm_ghosts[idx];
  if (g.moving) {
    pm_eraseGhost(idx);
    if (g.x < g.targetX) g.x++;
    else if (g.x > g.targetX) g.x--;
    if (g.y < g.targetY) g.y++;
    else if (g.y > g.targetY) g.y--;
    pm_drawGhost(idx);
    if (g.x == g.targetX && g.y == g.targetY) g.moving = false;
  } else {
    char newDir = pm_pickGhostDir(g);
    g.dir = newDir;
    int nr = g.row, nc = g.col;
    switch (newDir) {
      case 'U': nr--; break; case 'D': nr++; break;
      case 'L': nc--; break; case 'R': nc++; break;
    }
    if (pm_inBounds(nr, nc) && !pm_isWall(nr, nc)) {
      g.row = nr; g.col = nc;
      g.targetX = nc * PM_CELL_SIZE;
      g.targetY = PM_MAZE_Y + nr * PM_CELL_SIZE;
      g.moving = true;
    }
  }
}

char pm_oppositeDir(char d) {
  switch (d) { case 'U': return 'D'; case 'D': return 'U';
               case 'L': return 'R'; case 'R': return 'L'; }
  return 0;
}

char pm_pickGhostDir(PMGhost& g) {
  char reverse = pm_oppositeDir(g.dir);
  char dirs[4] = {'U','D','L','R'};
  long bestScore = 0x7FFFFFFF;
  char bestDir = g.dir;
  bool found = false;
  char fallback = 0;

  for (int i = 0; i < 4; i++) {
    char d = dirs[i];
    int nr = g.row, nc = g.col;
    switch (d) {
      case 'U': nr--; break; case 'D': nr++; break;
      case 'L': nc--; break; case 'R': nc++; break;
    }
    if (!pm_inBounds(nr, nc) || pm_isWall(nr, nc)) continue;
    if (d == reverse) { fallback = d; continue; }

    long sc;
    if (g.vulnerable) {
      sc = -((long)abs(nr - pm_row) + abs(nc - pm_col));
    } else if (g.aiType == 2) {
      sc = random(10000);
    } else if (g.aiType == 1) {
      int tr = pm_row, tc = pm_col;
      switch (pm_dir) {
        case 'U': tr -= 2; break; case 'D': tr += 2; break;
        case 'L': tc -= 2; break; case 'R': tc += 2; break;
      }
      sc = (long)abs(nr - tr) + abs(nc - tc);
    } else {
      sc = (long)abs(nr - pm_row) + abs(nc - pm_col);
    }
    if (!found || sc < bestScore) {
      bestScore = sc; bestDir = d; found = true;
    }
  }
  if (!found) return fallback ? fallback : g.dir;
  return bestDir;
}

void pm_checkCollisions() {
  for (int i = 0; i < PM_NUM_GHOSTS; i++) {
    if (pm_ghosts[i].row == pm_row && pm_ghosts[i].col == pm_col) {
      if (pm_ghosts[i].vulnerable) {
        pm_score += 200;
        pm_eraseGhost(i);
        pm_ghosts[i].row = pm_ghosts[i].spawnRow;
        pm_ghosts[i].col = pm_ghosts[i].spawnCol;
        pm_ghosts[i].x   = pm_ghosts[i].col * PM_CELL_SIZE;
        pm_ghosts[i].y   = PM_MAZE_Y + pm_ghosts[i].row * PM_CELL_SIZE;
        pm_ghosts[i].targetX = pm_ghosts[i].x;
        pm_ghosts[i].targetY = pm_ghosts[i].y;
        pm_ghosts[i].moving = false;
        pm_ghosts[i].vulnerable = false;
        pm_drawGhost(i);
        pm_drawHUD();
        playTone(2000, 150);
      } else {
        pm_lives--;
        pm_drawHUD();
        playTone(300, 300);
        delay(400);
        if (pm_lives <= 0) {
          pm_state = PM_GAMEOVER;
          pm_drawGameOverScreen();
          return;
        } else {
          delay(400);
          pm_initPac();
          pm_initGhosts();
          pm_drawGameStatic();
        }
        return;
      }
    }
  }
}

// ----- Drawing -----
void pm_drawMaze() {
  for (int r = 0; r < PM_GRID_ROWS; r++) {
    for (int c = 0; c < PM_GRID_COLS; c++) {
      if (pm_maze[r][c] == '#') {
        tft.fillRect(c * PM_CELL_SIZE, PM_MAZE_Y + r * PM_CELL_SIZE,
                     PM_CELL_SIZE, PM_CELL_SIZE, COL_BLUE);
      }
    }
  }
}

void pm_drawAllPellets() {
  for (int r = 0; r < PM_GRID_ROWS; r++) {
    for (int c = 0; c < PM_GRID_COLS; c++) {
      if (pm_pellets[r][c]) pm_drawPellet(r, c);
      if (pm_power[r][c])   pm_drawPowerPellet(r, c);
    }
  }
}

void pm_drawPellet(int r, int c) {
  tft.fillRect(c * PM_CELL_SIZE + 3, PM_MAZE_Y + r * PM_CELL_SIZE + 3, 2, 2, COL_WHITE);
}

void pm_drawPowerPellet(int r, int c) {
  tft.fillCircle(c * PM_CELL_SIZE + PM_CELL_SIZE / 2,
                 PM_MAZE_Y + r * PM_CELL_SIZE + PM_CELL_SIZE / 2,
                 3, COL_WHITE);
}

void pm_drawHUD() {
  tft.fillRect(0, 0, 160, 14, COL_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(COL_WHITE);
  tft.setCursor(2, 3);
  tft.print("SCORE:");
  tft.setTextColor(COL_YELLOW);
  tft.setCursor(38, 3);
  tft.print(pm_score);
  tft.setTextColor(COL_WHITE);
  tft.setCursor(85, 3);
  tft.print("LIFE:");
  for (int i = 0; i < pm_lives; i++) {
    int x = 116 + i * 10;
    tft.fillCircle(x, 7, 3, COL_YELLOW);
    tft.fillTriangle(x, 7, x + 4, 5, x + 4, 9, COL_BLACK);
  }
}

void pm_erasePac() {
  tft.fillRect(pm_x, pm_y, PM_CELL_SIZE, PM_CELL_SIZE, COL_BLACK);
  if (pm_pellets[pm_row][pm_col]) pm_drawPellet(pm_row, pm_col);
  if (pm_power[pm_row][pm_col])   pm_drawPowerPellet(pm_row, pm_col);
}

void pm_drawPac() {
  int cx = pm_x + PM_CELL_SIZE / 2;
  int cy = pm_y + PM_CELL_SIZE / 2;
  tft.fillCircle(cx, cy, 3, COL_YELLOW);
  if (pm_chompFrame == 0) {
    switch (pm_dir) {
      case 'R': tft.fillTriangle(cx, cy, cx + 4, cy - 3, cx + 4, cy + 3, COL_BLACK); break;
      case 'L': tft.fillTriangle(cx, cy, cx - 4, cy - 3, cx - 4, cy + 3, COL_BLACK); break;
      case 'U': tft.fillTriangle(cx, cy, cx - 3, cy - 4, cx + 3, cy - 4, COL_BLACK); break;
      case 'D': tft.fillTriangle(cx, cy, cx - 3, cy + 4, cx + 3, cy + 4, COL_BLACK); break;
    }
  }
}

void pm_eraseGhost(int idx) {
  PMGhost& g = pm_ghosts[idx];
  tft.fillRect(g.x, g.y, PM_CELL_SIZE, PM_CELL_SIZE, COL_BLACK);
  if (pm_pellets[g.row][g.col]) pm_drawPellet(g.row, g.col);
  if (pm_power[g.row][g.col])   pm_drawPowerPellet(g.row, g.col);
}

void pm_drawGhost(int idx) {
  PMGhost& g = pm_ghosts[idx];
  uint16_t c = g.vulnerable ? COL_DARKBLUE : g.color;
  int x = g.x, y = g.y;
  tft.fillCircle(x + 4, y + 3, 3, c);
  tft.fillRect(x + 1, y + 3, 6, 4, c);
  tft.drawPixel(x + 1, y + 7, c);
  tft.drawPixel(x + 3, y + 7, c);
  tft.drawPixel(x + 5, y + 7, c);
  tft.drawPixel(x + 7, y + 7, c);
  tft.fillRect(x + 1, y + 2, 2, 2, COL_WHITE);
  tft.fillRect(x + 5, y + 2, 2, 2, COL_WHITE);
  int px = x + 2, py = y + 3;
  switch (g.dir) {
    case 'L': px = x + 1; break;
    case 'R': px = x + 2; break;
    case 'U': py = y + 2; break;
    case 'D': py = y + 3; break;
  }
  tft.drawPixel(px, py, COL_BLUE);
  tft.drawPixel(px + 4, py, COL_BLUE);
}

void pm_drawGameOverScreen() {
  tft.fillScreen(COL_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(COL_RED);
  tft.setCursor(20, 30);
  tft.print("GAME OVER");
  tft.setTextSize(1);
  tft.setTextColor(COL_WHITE);
  tft.setCursor(50, 65);
  tft.print("Score: ");
  tft.setTextColor(COL_YELLOW);
  tft.print(pm_score);
  tft.setTextColor(COL_CYAN);
  tft.setCursor(15, 90);
  tft.print("Any key=Retry");
  tft.setCursor(15, 105);
  tft.print("* = Main Menu");
}

void pm_drawWinScreen() {
  tft.fillScreen(COL_BLACK);
  tft.setTextSize(2);
  tft.setTextColor(COL_YELLOW);
  tft.setCursor(25, 25);
  tft.print("YOU WIN!");
  tft.setTextSize(1);
  tft.setTextColor(COL_WHITE);
  tft.setCursor(50, 60);
  tft.print("Score: ");
  tft.setTextColor(COL_YELLOW);
  tft.print(pm_score);
  tft.setTextColor(COL_GREEN);
  tft.setCursor(15, 80);
  tft.print("Well done BATIKAN!");
  tft.setTextColor(COL_CYAN);
  tft.setCursor(15, 105);
  tft.print("* = Main Menu");
}


// #############################################################
// #                  FLAPPY FRIENDS GAME                      #
// #############################################################

const int FL_SCREEN_W   = 160;
const int FL_SCREEN_H   = 128;
const int FL_GROUND_H   = 14;
const int FL_PLAY_H     = FL_SCREEN_H - FL_GROUND_H;
const int FL_BIRD_SIZE  = 32;
const int FL_BIRD_X     = 25;
const float FL_GRAVITY  = 0.45f;
const float FL_FLAP_VY  = -3.6f;
const float FL_MAX_VY   = 6.0f;
const int FL_PIPE_W     = 20;
const int FL_PIPE_GAP   = 58;
const int FL_PIPE_SPACING = 86;
const int FL_NUM_PIPES  = 3;
const int FL_PHYSICS_MS = 33;

#define FL_TRANSPARENT 0xFFFF

const int FL_NUM_CHARS = 7;
const int FL_CHAR_NUMS[FL_NUM_CHARS] = {1, 2, 3, 4, 5, 6, 7};
const char* FL_CHAR_NAMES[FL_NUM_CHARS] = {
  "IREM", "HAN", "BIGBOY", "BATIKAN", "AZRA", "AYCA", "ARDA"
};

Adafruit_Image fl_charSprites[FL_NUM_CHARS][3];
bool fl_spriteOK[FL_NUM_CHARS][3];
bool fl_spritesLoaded = false;
int  fl_loadedCount = 0;

int fl_selectedChar = 0;

float fl_birdY;
float fl_birdVY;
int   fl_birdFrame;
int   fl_prevBirdY;
unsigned long fl_flapAnimUntil = 0;

struct FLPipe {
  int x, gapY;
  bool scored, active;
};
FLPipe fl_pipes[FL_NUM_PIPES];

int fl_score = 0;
int fl_bestScore = 0;
int fl_pipeSpeed = 1;

// FL states: INTRO is shown when launching from menu (only first time per session)
enum FLState { FL_INTRO, FL_SELECT, FL_PLAYING, FL_GAMEOVER };
FLState fl_state = FL_INTRO;
unsigned long fl_lastPhysics = 0;
unsigned long fl_lastBlink = 0;
bool fl_blinkOn = true;

// Forward decls
void fl_loadSprites();
void fl_drawSelectScreen();
void fl_drawBird();
void fl_eraseBird();
void fl_drawPipe(FLPipe& p);
void fl_drawIntro();

// ----- Entry from launcher -----
void flappy_enter() {
  // Lazy-load sprites once per session
  if (!fl_spritesLoaded) {
    if (!sdReady) {
      // SD failed - show error then go back
      tft.fillScreen(COL_BLACK);
      tft.setTextSize(2);
      tft.setTextColor(COL_RED);
      tft.setCursor(15, 25);
      tft.print("NO SD CARD");
      tft.setTextSize(1);
      tft.setTextColor(COL_WHITE);
      tft.setCursor(10, 60);
      tft.print("Flappy Friends needs SD");
      tft.setCursor(10, 72);
      tft.print("card with /images/ folder");
      tft.setTextColor(COL_GREEN);
      tft.setCursor(10, 100);
      tft.print("Press any key...");
      while (!keypad.getKey()) delay(20);
      enterState(S_MYGAMES);
      return;
    }
    fl_loadSprites();
  }
  fl_state = FL_INTRO;
  fl_lastBlink = millis();
  fl_blinkOn = true;
  fl_drawIntro();
}

void flappy_loop() {
  switch (fl_state) {
    case FL_INTRO:    fl_handleIntro();    break;
    case FL_SELECT:   fl_handleSelect();   break;
    case FL_PLAYING:  fl_handlePlaying();  break;
    case FL_GAMEOVER: fl_handleGameOver(); break;
  }
}

// ----- Sprite loading with on-screen progress -----
void fl_loadSprites() {
  tft.fillScreen(COL_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(COL_YELLOW);
  tft.setCursor(15, 5);
  tft.print("Loading sprites...");

  fl_loadedCount = 0;
  int total = FL_NUM_CHARS * 3;
  Serial.println("\n===== Flappy Sprite Loading =====");

  for (int i = 0; i < FL_NUM_CHARS; i++) {
    for (int j = 0; j < 3; j++) {
      char path[32];
      sprintf(path, "/images/h%d_r%d.bmp", FL_CHAR_NUMS[i], j + 1);
      Serial.print("  "); Serial.print(path); Serial.print(" -> ");
      ImageReturnCode r = reader.loadBMP(path, fl_charSprites[i][j]);
      if (r == IMAGE_SUCCESS) {
        fl_spriteOK[i][j] = true;
        fl_loadedCount++;
        Serial.println("OK");
      } else {
        fl_spriteOK[i][j] = false;
        Serial.print("FAIL: ");
        reader.printStatus(r);
      }
      // Update on-screen progress bar
      int pct = (i * 3 + j + 1) * 130 / total;
      tft.fillRect(15, 25, pct, 8, COL_GREEN);
      tft.drawRect(14, 24, 132, 10, COL_WHITE);
    }
  }

  Serial.print("Loaded "); Serial.print(fl_loadedCount);
  Serial.print("/"); Serial.println(total);

  // On-screen result summary
  tft.setTextColor(COL_WHITE);
  tft.setCursor(15, 50);
  tft.print("Loaded ");
  tft.setTextColor(fl_loadedCount == total ? COL_GREEN : COL_ORANGE);
  tft.print(fl_loadedCount);
  tft.print("/");
  tft.print(total);
  if (fl_loadedCount < total) {
    tft.setTextColor(COL_ORANGE);
    tft.setCursor(15, 65);
    tft.print("Some sprites failed.");
    tft.setCursor(15, 75);
    tft.print("Check Serial Monitor.");
    delay(2500);
  } else {
    delay(500);
  }

  fl_spritesLoaded = true;
}

// ----- Sprite drawing with white-to-bg substitution -----
void fl_drawSprite(int charIdx, int frameIdx, int x, int y, uint16_t bgColor) {
  if (!fl_spriteOK[charIdx][frameIdx]) {
    tft.fillRect(x, y, FL_BIRD_SIZE, FL_BIRD_SIZE, COL_PURPLE);
    tft.setTextSize(2);
    tft.setTextColor(COL_WHITE);
    tft.setCursor(x + 12, y + 9);
    tft.print("?");
    return;
  }
  GFXcanvas16* canvas = (GFXcanvas16*) fl_charSprites[charIdx][frameIdx].getCanvas();
  if (canvas == nullptr) {
    fl_charSprites[charIdx][frameIdx].draw(tft, x, y);
    return;
  }
  uint16_t* src = canvas->getBuffer();
  int w = canvas->width();
  int h = canvas->height();
  uint16_t tmp[FL_BIRD_SIZE * FL_BIRD_SIZE];
  if (w * h > FL_BIRD_SIZE * FL_BIRD_SIZE) {
    tft.startWrite();
    for (int py = 0; py < h; py++) {
      for (int px = 0; px < w; px++) {
        uint16_t pix = src[py * w + px];
        if (pix == FL_TRANSPARENT) pix = bgColor;
        tft.writePixel(x + px, y + py, pix);
      }
    }
    tft.endWrite();
    return;
  }
  for (int i = 0; i < w * h; i++) {
    tmp[i] = (src[i] == FL_TRANSPARENT) ? bgColor : src[i];
  }
  tft.drawRGBBitmap(x, y, tmp, w, h);
}

// ----- Intro screen (game-specific) -----
void fl_drawIntro() {
  tft.fillScreen(COL_SKY);
  tft.fillRect(0, FL_PLAY_H, FL_SCREEN_W, FL_GROUND_H, COL_GROUND);
  tft.fillRect(0, FL_PLAY_H, FL_SCREEN_W, 3, COL_GRASS);

  // Title with shadow
  tft.setTextSize(2);
  tft.setTextColor(COL_BLACK);
  tft.setCursor(22, 14);
  tft.print("FLAPPY");
  tft.setTextColor(COL_YELLOW);
  tft.setCursor(20, 12);
  tft.print("FLAPPY");

  tft.setTextColor(COL_BLACK);
  tft.setCursor(17, 34);
  tft.print("FRIENDS");
  tft.setTextColor(COL_GREEN);
  tft.setCursor(15, 32);
  tft.print("FRIENDS");

  // Decorative first character (with transparent bg)
  fl_drawSprite(0, 0, 115, 22, COL_SKY);

  // Mini decorative pipes
  tft.fillRect(60, 75, 8, 25, COL_PIPE_LIGHT_OR_GREEN());
  tft.fillRect(58, 91, 12, 4, COL_DARKGREEN);
  tft.fillRect(100, 75, 8, 25, COL_PIPE_LIGHT_OR_GREEN());
  tft.fillRect(98, 91, 12, 4, COL_DARKGREEN);
}

uint16_t COL_PIPE_LIGHT_OR_GREEN() { return COL_GREEN; }

void fl_handleIntro() {
  // Blinking text
  if (millis() - fl_lastBlink > 450) {
    fl_lastBlink = millis();
    fl_blinkOn = !fl_blinkOn;
    tft.setTextSize(1);
    tft.setCursor(28, 110);
    tft.setTextColor(fl_blinkOn ? COL_WHITE : COL_GROUND);
    tft.print("Press any key!");
  }

  // Minimum display time guard - prevents instant skip from buffered menu key
  if (millis() - stateStart < 800) {
    keypad.getKey();    // discard any pending key from menu
    return;
  }

  char k = keypad.getKey();
  if (k) {
    if (isBackKey(k)) {
      enterState(S_MYGAMES);
      return;
    }
    playTone(1500, 60);
    fl_state = FL_SELECT;
    fl_drawSelectScreen();
  }
}

// ----- Select screen -----
void fl_drawSelectScreen() {
  tft.fillScreen(COL_SKY);
  tft.fillRect(0, FL_PLAY_H, FL_SCREEN_W, FL_GROUND_H, COL_GROUND);
  tft.fillRect(0, FL_PLAY_H, FL_SCREEN_W, 3, COL_GRASS);

  tft.setTextSize(2);
  tft.setTextColor(COL_BLACK);
  tft.setCursor(12, 7);
  tft.print("PICK FRIEND");
  tft.setTextColor(COL_YELLOW);
  tft.setCursor(10, 5);
  tft.print("PICK FRIEND");

  fl_drawSelectedChar();

  tft.setTextSize(1);
  tft.setTextColor(COL_WHITE);
  tft.setCursor(8, 117);
  tft.print("S5< S7>  S1=START *=Menu");
}

void fl_drawSelectedChar() {
  tft.fillRect(0, 25, 160, FL_PLAY_H - 25, COL_SKY);
  int cx = 80, cy = 60;
  int x = cx - 16, y = cy - 16;

  tft.fillTriangle(15, cy, 28, cy - 10, 28, cy + 10, COL_GREEN);
  tft.fillTriangle(145, cy, 132, cy - 10, 132, cy + 10, COL_GREEN);

  tft.drawRect(x - 3, y - 3, FL_BIRD_SIZE + 6, FL_BIRD_SIZE + 6, COL_YELLOW);
  tft.drawRect(x - 2, y - 2, FL_BIRD_SIZE + 4, FL_BIRD_SIZE + 4, COL_YELLOW);

  fl_drawSprite(fl_selectedChar, 0, x, y, COL_SKY);

  tft.setTextSize(1);
  int nameLen = strlen(FL_CHAR_NAMES[fl_selectedChar]);
  int textX = 80 - (nameLen * 3);
  tft.setTextColor(COL_BLACK);
  tft.setCursor(textX + 1, 96);
  tft.print(FL_CHAR_NAMES[fl_selectedChar]);
  tft.setTextColor(COL_WHITE);
  tft.setCursor(textX, 95);
  tft.print(FL_CHAR_NAMES[fl_selectedChar]);

  tft.setTextColor(COL_BLACK);
  tft.setCursor(71, 106);
  tft.print(fl_selectedChar + 1);
  tft.print("/");
  tft.print(FL_NUM_CHARS);
}

void fl_handleSelect() {
  char k = keypad.getKey();
  if (!k) return;
  if (isBackKey(k)) {
    enterState(S_MYGAMES);
  } else if (k == '4') {
    fl_selectedChar = (fl_selectedChar - 1 + FL_NUM_CHARS) % FL_NUM_CHARS;
    playTone(1000, 30);
    fl_drawSelectedChar();
  } else if (k == '6') {
    fl_selectedChar = (fl_selectedChar + 1) % FL_NUM_CHARS;
    playTone(1000, 30);
    fl_drawSelectedChar();
  } else if (isSelectKey(k)) {
    playTone(1500, 60);
    fl_startGame();
  }
}

void fl_startGame() {
  fl_birdY     = FL_PLAY_H / 2;
  fl_birdVY    = 0;
  fl_birdFrame = 0;
  fl_prevBirdY = (int)fl_birdY;
  fl_score     = 0;
  fl_pipeSpeed = 1;

  for (int i = 0; i < FL_NUM_PIPES; i++) fl_pipes[i].active = false;
  fl_spawnPipe(0, FL_SCREEN_W + 20);
  fl_spawnPipe(1, FL_SCREEN_W + 20 + FL_PIPE_SPACING);

  fl_drawBackground();
  for (int i = 0; i < FL_NUM_PIPES; i++) if (fl_pipes[i].active) fl_drawPipe(fl_pipes[i]);
  fl_drawBird();
  fl_drawHUD();

  fl_state = FL_PLAYING;
}

void fl_drawBackground() {
  tft.fillRect(0, 0, FL_SCREEN_W, FL_PLAY_H, COL_SKY);
  tft.fillRect(0, FL_PLAY_H, FL_SCREEN_W, FL_GROUND_H, COL_GROUND);
  tft.fillRect(0, FL_PLAY_H, FL_SCREEN_W, 3, COL_GRASS);
  fl_drawCloud(25, 18);
  fl_drawCloud(95, 30);
  fl_drawCloud(135, 14);
}

void fl_drawCloud(int cx, int cy) {
  tft.fillCircle(cx,     cy,     5, COL_WHITE);
  tft.fillCircle(cx + 6, cy - 2, 4, COL_WHITE);
  tft.fillCircle(cx + 11, cy,    5, COL_WHITE);
  tft.fillRect(cx - 4, cy, 17, 5, COL_WHITE);
}

void fl_spawnPipe(int idx, int xPos) {
  fl_pipes[idx].x      = xPos;
  fl_pipes[idx].gapY   = 12 + random(FL_PLAY_H - 24 - FL_PIPE_GAP);
  fl_pipes[idx].scored = false;
  fl_pipes[idx].active = true;
}

void fl_updateDifficulty() {
  int n = 1 + fl_score / 7;
  if (n > 3) n = 3;
  fl_pipeSpeed = n;
}

void fl_handlePlaying() {
  char k = keypad.getKey();
  if (k) {
    if (isBackKey(k)) {
      enterState(S_MYGAMES);
      return;
    }
    fl_birdVY = FL_FLAP_VY;
    fl_birdFrame = 1;
    fl_flapAnimUntil = millis() + 220;
    playTone(800, 35);
  }

  if (millis() - fl_lastPhysics < FL_PHYSICS_MS) return;
  fl_lastPhysics = millis();

  fl_eraseBird();
  fl_birdVY += FL_GRAVITY;
  if (fl_birdVY > FL_MAX_VY) fl_birdVY = FL_MAX_VY;
  fl_birdY  += fl_birdVY;

  if (millis() > fl_flapAnimUntil && fl_birdFrame == 1) fl_birdFrame = 0;

  if (fl_birdY < 0) { fl_birdY = 0; fl_birdVY = 0; }
  if (fl_birdY > FL_PLAY_H - FL_BIRD_SIZE) {
    fl_doDeath();
    return;
  }

  for (int i = 0; i < FL_NUM_PIPES; i++) {
    if (!fl_pipes[i].active) continue;
    tft.fillRect(fl_pipes[i].x - 2, 0, FL_PIPE_W + 4, FL_PLAY_H, COL_SKY);
    fl_pipes[i].x -= fl_pipeSpeed;
    fl_drawPipe(fl_pipes[i]);

    if (!fl_pipes[i].scored && fl_pipes[i].x + FL_PIPE_W < FL_BIRD_X) {
      fl_pipes[i].scored = true;
      fl_score++;
      fl_updateDifficulty();
      fl_drawHUD();
      playTone(1400, 70);
    }
    if (fl_pipes[i].x + FL_PIPE_W < 0) { fl_pipes[i].active = false; continue; }
    if (fl_pipes[i].x < FL_BIRD_X + FL_BIRD_SIZE && fl_pipes[i].x + FL_PIPE_W > FL_BIRD_X) {
      int birdTop = (int)fl_birdY;
      int birdBot = birdTop + FL_BIRD_SIZE;
      if (birdTop < fl_pipes[i].gapY || birdBot > fl_pipes[i].gapY + FL_PIPE_GAP) {
        fl_doDeath();
        return;
      }
    }
  }

  int rightmost = -1000;
  for (int i = 0; i < FL_NUM_PIPES; i++) {
    if (fl_pipes[i].active && fl_pipes[i].x > rightmost) rightmost = fl_pipes[i].x;
  }
  if (rightmost < FL_SCREEN_W - FL_PIPE_SPACING) {
    for (int i = 0; i < FL_NUM_PIPES; i++) {
      if (!fl_pipes[i].active) { fl_spawnPipe(i, FL_SCREEN_W); break; }
    }
  }

  fl_drawBird();
}

void fl_doDeath() {
  fl_birdFrame = 2;
  playTone(400, 120); delay(130); playTone(250, 180);
  fl_birdVY = -2.0f;
  while (fl_birdY < FL_PLAY_H - FL_BIRD_SIZE) {
    fl_eraseBird();
    fl_birdVY += FL_GRAVITY * 1.8f;
    fl_birdY  += fl_birdVY;
    if (fl_birdY < 0) fl_birdY = 0;
    if (fl_birdY > FL_PLAY_H - FL_BIRD_SIZE) fl_birdY = FL_PLAY_H - FL_BIRD_SIZE;
    fl_drawBird();
    delay(35);
  }
  delay(500);
  fl_state = FL_GAMEOVER;
  fl_drawGameOverScreen();
}

void fl_drawPipe(FLPipe& p) {
  if (p.gapY > 0) {
    tft.fillRect(p.x, 0, FL_PIPE_W, p.gapY, COL_GREEN);
    tft.fillRect(p.x - 2, p.gapY - 5, FL_PIPE_W + 4, 5, COL_DARKGREEN);
  }
  int botY = p.gapY + FL_PIPE_GAP;
  if (botY < FL_PLAY_H) {
    tft.fillRect(p.x, botY, FL_PIPE_W, FL_PLAY_H - botY, COL_GREEN);
    tft.fillRect(p.x - 2, botY, FL_PIPE_W + 4, 5, COL_DARKGREEN);
  }
}

void fl_eraseBird() {
  tft.fillRect(FL_BIRD_X, fl_prevBirdY, FL_BIRD_SIZE, FL_BIRD_SIZE, COL_SKY);
}

void fl_drawBird() {
  int y = (int)fl_birdY;
  fl_drawSprite(fl_selectedChar, fl_birdFrame, FL_BIRD_X, y, COL_SKY);
  fl_prevBirdY = y;
}

void fl_drawHUD() {
  tft.fillRect(0, 0, 75, 12, COL_BLACK);
  tft.setTextSize(1);
  tft.setTextColor(COL_WHITE);
  tft.setCursor(2, 2);
  tft.print("Score:");
  tft.setTextColor(COL_YELLOW);
  tft.setCursor(38, 2);
  tft.print(fl_score);

  tft.fillRect(118, 0, 42, 12, COL_BLACK);
  tft.setTextColor(COL_GREEN);
  tft.setCursor(122, 2);
  tft.print("LVL:");
  tft.print(fl_pipeSpeed);
}

void fl_drawGameOverScreen() {
  bool newRecord = false;
  if (fl_score > fl_bestScore) { fl_bestScore = fl_score; newRecord = true; }

  tft.fillScreen(COL_SKY);
  tft.fillRect(0, FL_PLAY_H, FL_SCREEN_W, FL_GROUND_H, COL_GROUND);
  tft.fillRect(0, FL_PLAY_H, FL_SCREEN_W, 3, COL_GRASS);

  tft.setTextSize(2);
  tft.setTextColor(COL_BLACK);
  tft.setCursor(17, 10);
  tft.print("GAME OVER");
  tft.setTextColor(COL_RED);
  tft.setCursor(15, 8);
  tft.print("GAME OVER");

  fl_drawSprite(fl_selectedChar, 2, 64, 35, COL_SKY);

  tft.setTextSize(1);
  tft.setTextColor(COL_BLACK);
  tft.setCursor(46, 79);
  tft.print("Score: "); tft.print(fl_score);
  tft.setTextColor(COL_WHITE);
  tft.setCursor(45, 78);
  tft.print("Score: ");
  tft.setTextColor(COL_YELLOW);
  tft.print(fl_score);

  tft.setTextColor(COL_BLACK);
  tft.setCursor(46, 93);
  tft.print("Best:  "); tft.print(fl_bestScore);
  tft.setTextColor(COL_WHITE);
  tft.setCursor(45, 92);
  tft.print("Best:  ");
  tft.setTextColor(COL_GREEN);
  tft.print(fl_bestScore);

  if (newRecord) {
    tft.setTextColor(COL_ORANGE);
    tft.setCursor(45, 103);
    tft.print("NEW RECORD!");
  }

  tft.setTextColor(COL_WHITE);
  tft.setCursor(8, 117);
  tft.print("Any=Retry  *=Menu");
}

void fl_handleGameOver() {
  char k = keypad.getKey();
  if (!k) return;
  delay(150);
  if (isBackKey(k)) {
    enterState(S_MYGAMES);
  } else {
    fl_state = FL_SELECT;
    fl_drawSelectScreen();
  }
}