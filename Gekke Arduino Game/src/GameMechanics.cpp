#include <GameMechanics.h>


// Global Variables
extern Adafruit_ILI9341 tft;
extern int grid[GRID_ROWS][GRID_COLUMNS]; // Grid to hold Treasures
extern bool revealed[GRID_ROWS][GRID_COLUMNS]; // Keeps track of whether a cell has been dug
extern uint16_t cursorBuffer[BUFFER_SIZE]; // Buffer for background under the cursor
extern uint8_t player1Score; // Tracks Player 1's score
extern uint8_t player2Score; // Tracks Player 2's score


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

    // Calculate the new bounding box of the cursor
    int newGridXStart = (*posXp - RADIUS_PLAYER - (4 * 20)) / 20;
    int newGridXEnd = (*posXp + RADIUS_PLAYER - (4 * 20)) / 20;
    int newGridYStart = (*posYp - RADIUS_PLAYER) / 20;
    int newGridYEnd = (*posYp + RADIUS_PLAYER) / 20;

    // Check if the cursor is in a new grid cell
    if (newGridXStart != oldGridXStart || newGridXEnd != oldGridXEnd ||
        newGridYStart != oldGridYStart || newGridYEnd != oldGridYEnd) {
        
        // Restore the old bounding box
        for (int gridX = oldGridXStart; gridX <= oldGridXEnd; gridX++) {
            for (int gridY = oldGridYStart; gridY <= oldGridYEnd; gridY++) {
                if (gridX >= 0 && gridX < GRID_COLUMNS && gridY >= 0 && gridY < GRID_ROWS) {
                    int cellX = 4 * 20 + gridX * 20;
                    int cellY = gridY * 20;

                    // Restore background
                    tft.fillRect(cellX + 1, cellY + 1, 19, 19, ILI9341_WHITE);

                    // Restore grid lines
                    tft.drawLine(cellX, cellY, cellX + 20, cellY, ILI9341_BLACK); // Horizontal line
                    tft.drawLine(cellX, cellY, cellX, cellY + 20, ILI9341_BLACK); // Vertical line

                    // Restore numbers and Treasures
                    if (revealed[gridY][gridX]) {
                        if (grid[gridY][gridX] == 1) {
                            tft.fillRect(cellX + 5, cellY + 5, 10, 10, ILI9341_BLACK); // Treasure
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

        // Update old bounding box
        oldGridXStart = newGridXStart;
        oldGridXEnd = newGridXEnd;
        oldGridYStart = newGridYStart;
        oldGridYEnd = newGridYEnd;
    }

    // Read the joystick state
    Nunchuk.getState(NUNCHUK_ADDRESS);

    // Deadzone for joystickmovement
    int deadZone = 10;
    if (abs((int)Nunchuk.state.joy_x_axis - 127) > deadZone) {
        *posXp += (Nunchuk.state.joy_x_axis - 127) / 32;
    }
    if (abs((int)Nunchuk.state.joy_y_axis - 127) > deadZone) {
        *posYp += ((-Nunchuk.state.joy_y_axis + 255) - 127) / 32;
    }

    // Keep the cursor within the screen
    *posXp = constrain(*posXp, RADIUS_PLAYER + 4 * 20, 320 - RADIUS_PLAYER - 1);
    *posYp = constrain(*posYp, RADIUS_PLAYER, 240 - RADIUS_PLAYER - 1);

    // Draw new cursor
    tft.fillCircle(*posXp, *posYp, RADIUS_PLAYER, ILI9341_BLUE);

    // Check if the cursor is on a valid grid position
    int cursorGridX = (*posXp - (4 * 20)) / 20;
    int cursorGridY = *posYp / 20;

    if (cursorGridX >= 0 && cursorGridX < GRID_COLUMNS && cursorGridY >= 0 && cursorGridY < GRID_ROWS) {
        int cellX = 4 * 20 + cursorGridX * 20;
        int cellY = cursorGridY * 20;

        // If the cell is revealed, show the number of adjacent Treasures or the Treasure
        if (revealed[cursorGridY][cursorGridX]) {
            if (grid[cursorGridY][cursorGridX] == 1) {
                // Draw the Treasure
                tft.fillRect(cellX + 5, cellY + 5, 10, 10, ILI9341_BLACK);
            } else {
                // Draw the number of adjacent Treasures
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

    // Walk through all adjacent cells
    for (int offsetX = -1; offsetX <= 1; offsetX++) {
        for (int offsetY = -1; offsetY <= 1; offsetY++) {
            // calculate the coordinates of the neighbor cell
            int neighborX = gridX + offsetX;
            int neighborY = gridY + offsetY;

            // Check if the neighbor cell is within the grid
            if (neighborX >= 0 && neighborX < GRID_COLUMNS &&
                neighborY >= 0 && neighborY < GRID_ROWS) {
                // Only count Treasures
                if (grid[neighborY][neighborX] == 1) {
                    count++;
                }
            }
        }
    }

    return count; // return all adjacent Treasures
}


// Function to generate treasures
void generateTreasures() {
    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLUMNS; col++) {
            grid[row][col] = 0;      // no Treasure
            revealed[row][col] = false; // row and column are not revealed
        }
    }

    int TreasuresPlaced = 0;
    while (TreasuresPlaced < TREASURE_COUNT) {
        int randomRow = random(0, GRID_ROWS);
        int randomCol = random(0, GRID_COLUMNS);

        if (grid[randomRow][randomCol] == 0) {
            grid[randomRow][randomCol] = 1; // Place Treasure
            TreasuresPlaced++;
        }
    }
}

// Function to display treasures
void drawTreasures() {
    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLUMNS; col++) {
            if (grid[row][col] == 1) {
                int x = 4 * 20 + col * 20; // Calculate x-coordinate
                int y = row * 20;          // Calculate y-coordinate
                tft.fillRect(x + 5, y + 5, 10, 10, ILI9341_BLACK); // Treasure
            }
        }
    }
}

// Function to dig a cell
void digAction(uint16_t posX, uint16_t posY) {
    int gridX = (posX - (4 * 20)) / 20;
    int gridY = posY / 20;

    // Check if within the grid and not revealed yet
    if (gridX >= 0 && gridX < GRID_COLUMNS && gridY >= 0 && gridY < GRID_ROWS) {
        if (revealed[gridY][gridX]) {
            return;
        }
        revealed[gridY][gridX] = true;

        // Reveal the mine or the number of adjacent mines
        if (grid[gridY][gridX] == 1) {
            // Game over, mine dug
            int TreasureX = 4 * 20 + gridX * 20; //Pixelposition for the mine
            int TreasureY = gridY * 20;
            tft.fillRect(TreasureX + 5, TreasureY + 5, 10, 10, ILI9341_BLACK); // Draw the mine
            // Increment player 1 score (if desired action occurs)
            player1Score++;  // Increment score when the player successfully digs a safe cell
        } else {
            // No mine, dig the cell
            int digX = 4 * 20 + gridX * 20; // Pixelpositiion for the cell
            int digY = gridY * 20;
            tft.fillRect(digX + 1, digY + 1, 18, 18, ILI9341_WHITE); // Make the cell white

            // Count the number of adjacent mines
            int TreasureCount = countAdjacentTreasures(gridX, gridY);

            // Show the number of adjacent mines, including the number "0"
            tft.setCursor(digX + 6, digY + 3); // Center the number
            tft.setTextSize(1);
            tft.setTextColor(ILI9341_BLACK);
            tft.print(TreasureCount); // Always print the number, even if it is 0
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
    tft.fillRect(55, 55, 25, 40, backgroundColor); // Clear the player 1 and player 2 scores

    tft.setCursor(5, 65);  // Position for "Player 1:"
    tft.print("Player 1: "); // Print the score of Player 1
    tft.print(player1Score); // Print the score of Player 1

    tft.setCursor(5, 85);  // Position for "Player 2:"
    tft.print("Player 2:");
  }
}