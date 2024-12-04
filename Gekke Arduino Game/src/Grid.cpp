#include "Grid.h"

extern Adafruit_ILI9341 tft;

void SetupGrid(void)
{
  tft.setRotation(1);
  tft.fillScreen(ILI9341_WHITE); // Make the screen white
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(2);

  // Grid settings
  int cellSize = 20; // Size of a cell (in pixels)
  int screenWidth = 320; // Screen width (adjust for your screen)
  int screenHeight = 240; // Screen height (adjust for your screen)
  int x, y;

  // Reserve the first 4 columns for the scoreboard
  int scoreboardWidth = 4 * cellSize; // 4 columns for the scoreboard

  // Draw horizontal lines (no change to the y positions)
  for (y = 0; y <= screenHeight; y += cellSize)
  {
    tft.drawLine(scoreboardWidth, y, screenWidth, y, ILI9341_BLACK); // Start from scoreboardWidth to skip the left area
  }

  // Draw vertical lines (skip the first 4 columns for the scoreboard)
  for (x = scoreboardWidth; x <= screenWidth; x += cellSize)
  {
    tft.drawLine(x, 0, x, screenHeight, ILI9341_BLACK);
  }
}

void updateDisplay(uint16_t *posXp, uint16_t *posYp)
{
    static int oldGridXStart = -1, oldGridXEnd = -1;
    static int oldGridYStart = -1, oldGridYEnd = -1;

    // Bereken de nieuwe bounding box van de cursor
    int newGridXStart = (*posXp - RADIUS_PLAYER - (4 * 20)) / 20;
    int newGridXEnd = (*posXp + RADIUS_PLAYER - (4 * 20)) / 20;
    int newGridYStart = (*posYp - RADIUS_PLAYER) / 20;
    int newGridYEnd = (*posYp + RADIUS_PLAYER) / 20;

    // Controleer of de cursor in een nieuw gridvak is
    if (newGridXStart != oldGridXStart || newGridXEnd != oldGridXEnd ||
        newGridYStart != oldGridYStart || newGridYEnd != oldGridYEnd) {
        
        // Herstel de oude bounding box
        for (int gridX = oldGridXStart; gridX <= oldGridXEnd; gridX++) {
            for (int gridY = oldGridYStart; gridY <= oldGridYEnd; gridY++) {
                if (gridX >= 0 && gridX < GRID_COLUMNS && gridY >= 0 && gridY < GRID_ROWS) {
                    int cellX = 4 * 20 + gridX * 20;
                    int cellY = gridY * 20;

                    // Herstel achtergrond
                    tft.fillRect(cellX + 1, cellY + 1, 19, 19, ILI9341_WHITE);

                    // Herstel gridlijnen
                    tft.drawLine(cellX, cellY, cellX + 20, cellY, ILI9341_BLACK); // Horizontale lijn
                    tft.drawLine(cellX, cellY, cellX, cellY + 20, ILI9341_BLACK); // Verticale lijn
                }
            }
        }

        // Update de oude bounding box
        oldGridXStart = newGridXStart;
        oldGridXEnd = newGridXEnd;
        oldGridYStart = newGridYStart;
        oldGridYEnd = newGridYEnd;
    }

    // Lees Nunchuk-joystick
    Nunchuk.getState(NUNCHUK_ADDRESS);

    // Deadzone voor joystickbeweging
    int deadZone = 10;
    if ((unsigned int)abs(Nunchuk.state.joy_x_axis - 127) > deadZone) {
        *posXp += (Nunchuk.state.joy_x_axis - 127) / 32;
    }
    if ((unsigned int)abs(Nunchuk.state.joy_y_axis - 127) > deadZone) {
        *posYp += ((-Nunchuk.state.joy_y_axis + 255) - 127) / 32;
    }

    // Houd cursor binnen schermgrenzen
    *posXp = constrain(*posXp, RADIUS_PLAYER + 4 * 20, 320 - RADIUS_PLAYER - 1);
    *posYp = constrain(*posYp, RADIUS_PLAYER, 240 - RADIUS_PLAYER - 1);

    // Teken de nieuwe cursorpositie
    tft.fillCircle(*posXp, *posYp, RADIUS_PLAYER, ILI9341_BLUE);
}

void restoreOldCursorPosition(uint16_t oldX, uint16_t oldY)
{
    // Bereken de gridpositie
    int gridX = (oldX - (4 * 20)) / 20;
    int gridY = oldY / 20;

    // Controleer of de positie binnen het speelveld valt
    if (gridX >= 0 && gridX < GRID_COLUMNS && gridY >= 0 && gridY < GRID_ROWS) {
        int cellX = 4 * 20 + gridX * 20;
        int cellY = gridY * 20;

        // Herstel de achtergrondkleur van de cel
        tft.fillRect(cellX + 1, cellY + 1, 19, 19, ILI9341_WHITE);

        // Herstel gridlijnen
        tft.drawLine(cellX, cellY, cellX + 20, cellY, ILI9341_BLACK); // Bovenste lijn
        tft.drawLine(cellX, cellY, cellX, cellY + 20, ILI9341_BLACK); // Linker lijn
    }
}