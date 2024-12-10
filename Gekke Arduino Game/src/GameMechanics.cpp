#include <GameMechanics.h>
#include <HardwareSerial.h>

// Global Variables
extern Adafruit_ILI9341 tft;

int player1Score = 0; // Initialize player1Score
int player2Score = 0; // Initialize player2Score

uint8_t grid[GRID_SIZE][GRID_SIZE]; // Grid to hold Treasures
bool revealed[GRID_SIZE][GRID_SIZE]; // Keeps track of whether a cell has been dug
uint16_t cursorBuffer[BUFFER_SIZE]; // Buffer for background under the cursor
uint8_t cellSize = SCREEN_HEIGHT / GRID_SIZE;
uint8_t scoreboardWidth = 80;

void SetupGrid() {
    // Set up the display
    Nunchuk.begin(NUNCHUK_ADDRESS);
    tft.setRotation(1);
    tft.fillScreen(ILI9341_WHITE); // Make the screen white
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(2);

    // Draw horizontal lines
    for (int y = 0; y <= SCREEN_HEIGHT; y += cellSize) {
        tft.drawLine(scoreboardWidth, y, SCREEN_WIDTH, y, ILI9341_BLACK);
    }

    // Draw vertical lines (skip the first 4 columns for the scoreboard)
    for (int x = scoreboardWidth; x <= SCREEN_WIDTH; x += cellSize) {
        tft.drawLine(x, 0, x, SCREEN_HEIGHT, ILI9341_BLACK);
    }
    // Generate Treasures
    generateTreasures();

}


void updateDisplay(uint16_t *posXp, uint16_t *posYp)
{
    static int oldGridXStart = -1, oldGridXEnd = -1;
    static int oldGridYStart = -1, oldGridYEnd = -1;

    // Calculate the new bounding box of the cursor
    int newGridXStart = (*posXp - RADIUS_PLAYER - scoreboardWidth) / cellSize;
    int newGridXEnd = (*posXp + RADIUS_PLAYER - scoreboardWidth) / cellSize;
    int newGridYStart = (*posYp - RADIUS_PLAYER) / cellSize;
    int newGridYEnd = (*posYp + RADIUS_PLAYER) / cellSize;

    // Check if the cursor is in a new grid cell
    if (newGridXStart != oldGridXStart || newGridXEnd != oldGridXEnd ||
        newGridYStart != oldGridYStart || newGridYEnd != oldGridYEnd) {
        
        // Restore the old bounding box
        for (int gridX = oldGridXStart; gridX <= oldGridXEnd; gridX++) {
            for (int gridY = oldGridYStart; gridY <= oldGridYEnd; gridY++) {
                if (gridX >= 0 && gridX < GRID_SIZE && gridY >= 0 && gridY < GRID_SIZE) {
                    int cellX = scoreboardWidth + gridX * cellSize;
                    int cellY = gridY * cellSize;

                    // Restore background
                    tft.fillRect(cellX + 1, cellY + 1, cellSize-1, cellSize-1, ILI9341_WHITE);

                    // Restore grid lines
                    tft.drawLine(cellX, cellY, cellX + cellSize, cellY, ILI9341_BLACK); // Horizontal line
                    tft.drawLine(cellX, cellY, cellX, cellY + cellSize, ILI9341_BLACK); // Vertical line

                    // Restore numbers and Treasures
                    if (revealed[gridY][gridX]) {
                        if (grid[gridY][gridX] == 1) {
                            tft.fillRect(cellX + 5, cellY + 5, cellSize, cellSize, ILI9341_BLACK); // Treasure
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
    *posXp = constrain(*posXp, RADIUS_PLAYER + scoreboardWidth, SCREEN_WIDTH - RADIUS_PLAYER - 1);
    *posYp = constrain(*posYp, RADIUS_PLAYER, SCREEN_HEIGHT - RADIUS_PLAYER - 1);

    // Draw new cursor
    tft.fillCircle(*posXp, *posYp, RADIUS_PLAYER, ILI9341_BLUE);

    // Check if the cursor is on a valid grid position
    int cursorGridX = (*posXp - scoreboardWidth) / cellSize;
    int cursorGridY = *posYp / cellSize;

    if (cursorGridX >= 0 && cursorGridX < GRID_SIZE && cursorGridY >= 0 && cursorGridY < GRID_SIZE) {
        int cellX = scoreboardWidth + cursorGridX * cellSize;
        int cellY = cursorGridY * cellSize;

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
            if (neighborX >= 0 && neighborX < GRID_SIZE &&
                neighborY >= 0 && neighborY < GRID_SIZE) {
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
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            grid[row][col] = 0;      // no Treasure
            revealed[row][col] = false; // row and column are not revealed
        }
    }

    int TreasuresPlaced = 0;
    while (TreasuresPlaced < TREASURE_COUNT) {
        int randomRow = random(0, GRID_SIZE);
        int randomCol = random(0, GRID_SIZE);

        if (grid[randomRow][randomCol] == 0) {
            grid[randomRow][randomCol] = 1; // Place Treasure
            TreasuresPlaced++;
        }
    }
}

// Function to display treasures
void drawTreasures() {
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            if (grid[row][col] == 1) {
                int x = scoreboardWidth + col * cellSize; // Calculate x-coordinate
                int y = row * cellSize;          // Calculate y-coordinate
                tft.fillRect(x + 5, y + 5, 10, 10, ILI9341_BLACK); // Treasure
            }
        }
    }
}

// Function to dig a cell
void digAction(uint16_t posX, uint16_t posY) {
    int gridX = (posX - scoreboardWidth) / cellSize;
    int gridY = posY / cellSize;

    // Check if within the grid and not revealed yet
    if (gridX >= 0 && gridX < GRID_SIZE && gridY >= 0 && gridY < GRID_SIZE) {
        if (revealed[gridY][gridX]) {
            return;
        }
        revealed[gridY][gridX] = true;

        // Reveal the mine or the number of adjacent mines
        if (grid[gridY][gridX] == 1) {
            // Game over, mine dug
            int TreasureX = scoreboardWidth + gridX * cellSize; //Pixelposition for the mine
            int TreasureY = gridY * cellSize;
            tft.fillRect(TreasureX + 5, TreasureY + 5, 5, 5, ILI9341_BLACK); // Draw the mine
            // Increment player 1 score (if desired action occurs)
            player1Score++;  // Increment score when the player successfully digs a safe cell
        } else {
            // No mine, dig the cell
            int digX = scoreboardWidth + gridX * cellSize; // Pixelpositiion for the cell
            int digY = gridY * cellSize;
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
  int gridX = (posX - scoreboardWidth) / cellSize; // Subtract the space for the scoreboard
  int gridY = posY / cellSize;

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

bool isGameOver() {
    // Check if all treasures have been revealed
    int revealedTreasures = 0;
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            if (grid[row][col] == 1 && revealed[row][col]) {
                revealedTreasures++;
            }
        }
    }
    return revealedTreasures == TREASURE_COUNT;
}