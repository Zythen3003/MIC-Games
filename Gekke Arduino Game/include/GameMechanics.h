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
extern int player1Score;
extern int player2Score;
// Function Declarations
void SetupGrid();
void displayScoreboard(uint16_t posX, uint16_t posY);
int countAdjacentTreasures(int gridX, int gridY);
void updateDisplay(uint16_t *posXp, uint16_t *posYp);
void generateTreasures();
void drawTreasures();
void digAction(uint16_t posX, uint16_t posY, bool isPlayer1);


#endif