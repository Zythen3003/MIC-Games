#ifndef GRID_H
#define GRID_H

#include "Adafruit_ILI9341.h"
#include "Nunchuk.h"

#define RADIUS_PLAYER 5
#define GRID_COLUMNS 12 // Aantal kolommen in de grid
#define GRID_ROWS 12    // Aantal rijen in de grid
#define NUNCHUK_ADDRESS 0x52

extern Adafruit_ILI9341 tft;

extern int grid[GRID_ROWS][GRID_COLUMNS]; // Grid om mijnen bij te houden
extern bool revealed[GRID_ROWS][GRID_COLUMNS]; // Houdt bij of een vakje is gegraven

void SetupGrid();
void updateDisplay(uint16_t *posXp, uint16_t *posYp);
void restoreOldCursorPosition(uint16_t oldX, uint16_t oldY);

#endif