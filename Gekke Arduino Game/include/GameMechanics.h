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
#define TREASURE_COUNT 0   // Number of Treasures
#define BUFFER_SIZE (2 * RADIUS_PLAYER * 2 * RADIUS_PLAYER)
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Global variables
extern Adafruit_ILI9341 tft;
extern int grid[GRID_SIZE][GRID_SIZE];
extern bool revealed[GRID_SIZE][GRID_SIZE];
extern uint16_t cursorBuffer[BUFFER_SIZE];
extern uint8_t player1Score;
extern uint8_t player2Score;

// Function Declarations
void SetupGrid();
void displayScoreboard(uint16_t posX, uint16_t posY);
int countAdjacentTreasures(int gridX, int gridY);
void updateDisplay(uint16_t *posXp, uint16_t *posYp);
void generateTreasures();
void drawTreasures();
void digAction(uint16_t posX, uint16_t posY);

#endif