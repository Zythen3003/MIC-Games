#ifndef GAME_MECHANICS_H
#define GAME_MECHANICS_H

#include "Adafruit_ILI9341.h"
#include "Nunchuk.h"

// Define constants and variables for game mechanics
#define TFT_DC 9
#define TFT_CS 10
#define NUNCHUK_ADDRESS 0x52
#define RADIUS_PLAYER 5
#define GRID_SIZE 12    // Rows in the grid
#define TREASURE_COUNT 5   // Number of Treasures
#define BUFFER_SIZE (2 * RADIUS_PLAYER * 2 * RADIUS_PLAYER)
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Global variables
extern Adafruit_ILI9341 tft;

// Other game mechanics function prototypes
bool isGameOver();
extern uint8_t player1Score;
extern uint8_t player2Score;
extern bool isTreasure;
extern unsigned long gameTime;  // In-game time in seconds

// Function Declarations
void SetupGrid(int ticksSinceLastUpdate);
void displayScoreboard();
void updatePosition();
void updateTimer();
void updateScore();
uint8_t countAdjacentTreasures(uint8_t gridX, uint8_t gridY);
void generateTreasures(int ticksSinceLastUpdate);
void drawTreasures();
void digAction(bool isPlayer1);
void doGameLoop();
void movePlayer();
void gridToDisplayCoords(uint16_t &x, uint16_t &y);
void updateCell(bool isPlayer1, uint8_t gridX = 255, uint8_t gridY = 255);

#endif