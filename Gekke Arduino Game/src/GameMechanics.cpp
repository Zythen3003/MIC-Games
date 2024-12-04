#include <GameMechanics.h>


// Global Variables
extern Adafruit_ILI9341 tft;
extern int grid[GRID_ROWS][GRID_COLUMNS]; // Grid om mijnen bij te houden
extern bool revealed[GRID_ROWS][GRID_COLUMNS]; // Houdt bij of een vakje is gegraven
extern uint16_t cursorBuffer[BUFFER_SIZE]; // Buffer voor achtergrond onder de cursor
extern uint8_t player1Score; // Houd de score van Player 1 bij
extern uint8_t player2Score; // Houd de score van Player 2 bij

void SetupGrid(void)
{
    // Set up the display
    Nunchuk.begin(NUNCHUK_ADDRESS);
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
                    tft.fillRect(cellX + 1, cellY + 1, 18, 18, ILI9341_WHITE);

                    // Herstel gridlijnen
                    tft.drawLine(cellX, cellY, cellX + 20, cellY, ILI9341_BLACK); // Horizontale lijn
                    tft.drawLine(cellX, cellY, cellX, cellY + 20, ILI9341_BLACK); // Verticale lijn

                    // Herstel cijfers of mijnen
                    if (revealed[gridY][gridX]) {
                        if (grid[gridY][gridX] == 1) {
                            tft.fillRect(cellX + 5, cellY + 5, 10, 10, ILI9341_BLACK); // Mijn
                        } else {
                            int TreasureCount = countAdjacentTreasures(gridX, gridY);
                            tft.setCursor(cellX + 6, cellY + 3);
                            tft.setTextSize(1);
                            tft.setTextColor(ILI9341_BLACK);
                            tft.print(TreasureCount);
                        }
                    }
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
    if (abs((int)Nunchuk.state.joy_x_axis - 127) > deadZone) {
        *posXp += (Nunchuk.state.joy_x_axis - 127) / 32;
    }
    if (abs((int)Nunchuk.state.joy_y_axis - 127) > deadZone) {
        *posYp += ((-Nunchuk.state.joy_y_axis + 255) - 127) / 32;
    }

    // Houd cursor binnen schermgrenzen
    *posXp = constrain(*posXp, RADIUS_PLAYER + 4 * 20, 320 - RADIUS_PLAYER - 1);
    *posYp = constrain(*posYp, RADIUS_PLAYER, 240 - RADIUS_PLAYER - 1);

    // Teken de nieuwe cursorpositie
    tft.fillCircle(*posXp, *posYp, RADIUS_PLAYER, ILI9341_BLUE);

    // Controleer of de cursor zich in een geldig gridvak bevindt
    int cursorGridX = (*posXp - (4 * 20)) / 20;
    int cursorGridY = *posYp / 20;

    if (cursorGridX >= 0 && cursorGridX < GRID_COLUMNS && cursorGridY >= 0 && cursorGridY < GRID_ROWS) {
        int cellX = 4 * 20 + cursorGridX * 20;
        int cellY = cursorGridY * 20;

        // Als het vakje is gegraven, teken het cijfer of de mijn opnieuw
        if (revealed[cursorGridY][cursorGridX]) {
            if (grid[cursorGridY][cursorGridX] == 1) {
                // Teken mijn
                tft.fillRect(cellX + 5, cellY + 5, 10, 10, ILI9341_BLACK);
            } else {
                // Teken het aantal omliggende mijnen
                int TreasureCount = countAdjacentTreasures(cursorGridX, cursorGridY);
                tft.setCursor(cellX + 6, cellY + 3);
                tft.setTextSize(1);
                tft.setTextColor(ILI9341_BLACK);
                tft.print(TreasureCount);
            }
        }
    }
}




int countAdjacentTreasures(int gridX, int gridY) {
    int count = 0;

    // Loop door alle omliggende cellen
    for (int offsetX = -1; offsetX <= 1; offsetX++) {
        for (int offsetY = -1; offsetY <= 1; offsetY++) {
            // Bereken de coördinaten van de buurcel
            int neighborX = gridX + offsetX;
            int neighborY = gridY + offsetY;

            // Controleer of de buurcel binnen de grid valt
            if (neighborX >= 0 && neighborX < GRID_COLUMNS &&
                neighborY >= 0 && neighborY < GRID_ROWS) {
                // Tel alleen als er een mijn ligt
                if (grid[neighborY][neighborX] == 1) {
                    count++;
                }
            }
        }
    }

    return count; // Retourneer het aantal omliggende mijnen
}


// Functie om mijnen te genereren
void generateTreasures() {
    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLUMNS; col++) {
            grid[row][col] = 0;      // Geen mijn
            revealed[row][col] = false; // Vakje niet gegraven
        }
    }

    int TreasuresPlaced = 0;
    while (TreasuresPlaced < TREASURE_COUNT) {
        int randomRow = random(0, GRID_ROWS);
        int randomCol = random(0, GRID_COLUMNS);

        if (grid[randomRow][randomCol] == 0) {
            grid[randomRow][randomCol] = 1; // Plaats mijn
            TreasuresPlaced++;
        }
    }
}

// Functie om mijnen weer te geven
void drawTreasures() {
    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLUMNS; col++) {
            if (grid[row][col] == 1) {
                int x = 4 * 20 + col * 20; // Bereken x-coördinaat
                int y = row * 20;          // Bereken y-coördinaat
                tft.fillRect(x + 5, y + 5, 10, 10, ILI9341_BLACK); // Mijn
            }
        }
    }
}

void digAction(uint16_t posX, uint16_t posY) {
    // Bereken de huidige gridpositie van de cursor
    int gridX = (posX - (4 * 20)) / 20; // Bereken kolom
    int gridY = posY / 20;              // Bereken rij

    // Controleer of de gridpositie geldig is
    if (gridX >= 0 && gridX < GRID_COLUMNS && gridY >= 0 && gridY < GRID_ROWS) {
        if (revealed[gridY][gridX]) {
            // Dit vakje is al gegraven, doe niets
            return;
        }

        // Markeer het vakje als gegraven
        revealed[gridY][gridX] = true;

        if (grid[gridY][gridX] == 1) {
            // Er is een mijn - toon mijn en voer actie uit
            int TreasureX = 4 * 20 + gridX * 20; // Pixelpositie van mijn
            int TreasureY = gridY * 20;
            tft.fillRect(TreasureX + 5, TreasureY + 5, 10, 10, ILI9341_BLACK); // Teken mijn
            // Increment player 1 score (if desired action occurs)
            player1Score++;  // Increment score when the player successfully digs a safe cell
            //tft.fillScreen(ILI9341_RED); // Maak het scherm rood
            //tft.setCursor(100, 120);    // Toon "Game Over"
            //tft.setTextSize(3);
            //tft.setTextColor(ILI9341_WHITE);
            //tft.print("BOOM!");
            //while (true); // Stop het spel (of reset later)
        } else {
            // Geen mijn - graaf de cel vrij
            int digX = 4 * 20 + gridX * 20; // Pixelpositie voor de cel
            int digY = gridY * 20;
            tft.fillRect(digX + 1, digY + 1, 18, 18, ILI9341_WHITE); // Maak de cel wit

            // Tel omliggende mijnen
            int TreasureCount = countAdjacentTreasures(gridX, gridY);

            // Toon het aantal omliggende mijnen, inclusief 0
            tft.setCursor(digX + 6, digY + 3); // Centreer in de cel
            tft.setTextSize(1);
            tft.setTextColor(ILI9341_BLACK);
            tft.print(TreasureCount); // Print ook "0" als er geen omliggende mijnen zijn
        }
    }
}

void displayScoreboard(uint16_t posX, uint16_t posY) {
  // Static variables to remember the previous state
  static int lastGridX = -1;  // -1 to indicate initial value
  static int lastGridY = -1;  // -1 to indicate initial value
  static int lastScorePlayer1;  // -1 to indicate initial value

  // Calculate the grid position
  int gridX = (posX - (4 * 20)) / 20; // Subtract the space for the scoreboard
  int gridY = posY / 20;

  // Define the background color of the scoreboard (white or whatever background color you are using)
  uint16_t backgroundColor = ILI9341_WHITE; // Change to match your background color if needed

  // Display the scoreboard title and static information (this only needs to be done once)
  tft.setCursor(10, 45);  // Position for the "Scoreboard" title
  tft.setTextSize(1);
  tft.print("Scoreboard");

  tft.setCursor(5, 65);  // Position for "Player 1:"
  tft.print("Player 1: "); // Print the score of Player 1
  tft.print(player1Score); // Print the score of Player 1

  tft.setCursor(5, 85);  // Position for "Player 2:"
  tft.print("Player 2:");

  tft.setCursor(10, 115);  // Position for "Current"
  tft.print("Current ");
  tft.setCursor(10, 135); // Position for "Position"
  tft.print("Position");

  // Check if grid position has changed
  if (gridX != lastGridX || gridY != lastGridY) {
    // Clear previous grid position if it has changed
    tft.fillRect(15, 155, 25, 40, backgroundColor); // Clear the "X:" and "Y:" values

    // Update new X and Y grid positions
    tft.setCursor(10, 155);
    tft.print("X: ");
    tft.print(gridX + 1); // Add 1 to match the grid numbering (1-based)

    tft.setCursor(10, 175);
    tft.print("Y: ");
    tft.print(gridY + 1); // Add 1 to match the grid numbering (1-based)

    // Update last known grid positions
    lastGridX = gridX;
    lastGridY = gridY;

  }
  else if(player1Score != lastScorePlayer1)  // Check if player score has changed
  {
    // Clear previous grid position if it has changed
    tft.fillRect(50, 55, 20, 40, backgroundColor); // Clear the player 1 and player 2 scores

    tft.setCursor(5, 65);  // Position for "Player 1:"
    tft.print("Player 1: "); // Print the score of Player 1
    tft.print(player1Score); // Print the score of Player 1

    tft.setCursor(5, 85);  // Position for "Player 2:"
    tft.print("Player 2:");
  }
}


