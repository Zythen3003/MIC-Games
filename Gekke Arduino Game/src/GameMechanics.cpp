#include <GameMechanics.h>
#include <HardwareSerial.h>
#include <Multiplayer.h>

uint8_t player1Score = 0; // Initialize player1Score
uint8_t player1X;
uint8_t player1Y;

uint8_t grid[GRID_SIZE][GRID_SIZE];  // Grid to hold Treasures
bool revealed[GRID_SIZE][GRID_SIZE]; // Keeps track of whether a cell has been dug
uint8_t cellSize = SCREEN_HEIGHT / GRID_SIZE;
uint8_t scoreboardWidth = 80;

bool isTreasure = false; // Initialize isTreasure
bool joystickReset = false;

bool renderPlayer2 = false;

volatile unsigned long timerMillis = 0; // Millisecond counter
unsigned long gameTime = 0;             // In-game time in seconds
unsigned long lastUpdateTime = 0;       // Store time of the last update for event synchronization

// Timer2 initialization function
void Timer2_Init()
{
    // // Clear Timer2 control registers
    // TCCR2A = 0; // Normal mode
    // TCCR2B = 0; // Normal mode

    // // Set Timer2 prescaler to 64 (this gives an interrupt roughly every 4.096ms)
    // TCCR2B |= (1 << CS22); // Prescaler 64 (CS22 set)
    // TCCR2B |= (1 << CS21); // Prescaler 64 (CS21 set)

    // // Enable Timer2 overflow interrupt
    // TIMSK2 |= (1 << TOIE2); // Enable Timer2 overflow interrupt

    // // Initialize the timer counter to 0
    // TCNT2 = 0;
}

// Timer2 overflow interrupt handler
ISR(TIMER2_OVF_vect)
{
    timerMillis++; // Increment the millisecond counter on each overflow

    // Periodically update the game time (every 256ms, i.e., every 0.256 seconds in real time)
    if (timerMillis % (256) == 0)
    {               // Update every 256ms (approx. 0.256 real seconds)
        gameTime++; // Increment game time in seconds
        updateTimer();
    }
}

void SetupGrid(bool isSingleplayer)
{
    renderPlayer2 = !isSingleplayer;

    // Set up the display
    Nunchuk.begin(NUNCHUK_ADDRESS);
    tft.setRotation(1);
    tft.fillScreen(ILI9341_WHITE); // Make the screen white
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(2);
    // Set up Timer2 to interrupt every second
    Timer2_Init();
    gameTime = 0; // Reset the game time to 0

    // Draw horizontal lines
    for (int y = 0; y <= SCREEN_HEIGHT; y += cellSize)
    {
        tft.drawLine(scoreboardWidth, y, SCREEN_WIDTH, y, ILI9341_BLACK);
    }

    // Draw vertical lines (skip the first 4 columns for the scoreboard)
    for (int x = scoreboardWidth; x <= SCREEN_WIDTH; x += cellSize)
    {
        tft.drawLine(x, 0, x, SCREEN_HEIGHT, ILI9341_BLACK);
    }

    // Generate Treasures
    generateTreasures();
    displayScoreboard();
    
    updateCell(true);
    if (!isSingleplayer)
        updateCell(false); 
}

void doGameLoop()
{
    // updateCell(true);

    // // MULTIPLAYER moet nog verder gemaakt worden
    // if (false)
    // {
    //     updateCell(false);
    // }
}

void updateCell(bool isPlayer1, uint8_t gridX = 255, uint8_t gridY = 255)
{
    bool isPlayerCell = true;
    if (gridX != 255 || gridY != 255)
    {
        isPlayerCell = false;
    }

    gridX = (gridX != 255 ? gridX : (isPlayer1 ? player1X : player2X));
    gridY = (gridY != 255 ? gridY : (isPlayer1 ? player1Y : player2Y));

    uint16_t x = gridX;
    uint16_t y = gridY;

    gridToDisplayCoords(x, y);

    if (isPlayerCell)
    {
        if (isPlayer1)
            tft.fillCircle(x, y, RADIUS_PLAYER, ILI9341_BLUE);
        else
            tft.fillCircle(x, y, RADIUS_PLAYER, ILI9341_RED);
    }
    else
    {
        tft.fillRect((x - (cellSize / 2)) + 1, (y - (cellSize / 2)) + 1, cellSize - 2, cellSize - 2, ILI9341_WHITE); // Treasure
    }

    // If the cell is revealed, show the number of adjacent Treasures or the Treasure
    if (revealed[gridY][gridX])
    {
        if (grid[gridY][gridX] == 1)
        {
            // Treasure found
            tft.fillRect(x - (cellSize / 4), y - (cellSize / 4), cellSize - (cellSize / 2), cellSize - (cellSize / 2), ILI9341_BLACK); // Treasure
        }
        else
        {
            // No treasure found
            tft.fillRect(x - (cellSize / 4), y - (cellSize / 4), cellSize - (cellSize / 2), cellSize - (cellSize / 2), ILI9341_WHITE);

            if (isPlayerCell)
            {
                if (isPlayer1)
                    tft.fillCircle(x, y, RADIUS_PLAYER, ILI9341_BLUE);
                else
                    tft.fillCircle(x, y, RADIUS_PLAYER, ILI9341_RED);
            }

            // Count the number of adjacent mines
            uint8_t TreasureCount = countAdjacentTreasures(gridX, gridY);

            // Show the number of adjacent mines, including the number "0"
            tft.setCursor(x - (cellSize / 4), y - (cellSize / 4)); // Center the number
            tft.setTextSize(1);
            tft.setTextColor(ILI9341_BLACK);
            tft.print(TreasureCount); // Always print the number, even if it is 0
        }
    }
}

void gridToDisplayCoords(uint16_t &x, uint16_t &y)
{
    x = scoreboardWidth + ((x + 1) * cellSize) - (cellSize / 2);
    y = (y + 1) * cellSize - (cellSize / 2);
}

void movePlayer()
{
    // Check if the joystick has returned to the center
    uint8_t joystickThreshold = 60;
    uint8_t centre = 128;

    Nunchuk.getState(NUNCHUK_ADDRESS);

    if (abs(Nunchuk.state.joy_x_axis - centre) < joystickThreshold && abs(Nunchuk.state.joy_y_axis - centre) < joystickThreshold)
    {
        joystickReset = true;
    }

    // Only move if the joystick has reset to center
    if (joystickReset)
    {
        // Move up
        if (Nunchuk.state.joy_y_axis > centre + joystickThreshold && player1Y != 0)
        {
            updateCell(true, player1X, player1Y);
            player1Y -= 1;
            if (renderPlayer2)
                sendCommand(MoveUp);
            joystickReset = false;
        }
        // Move down
        else if (Nunchuk.state.joy_y_axis < centre - joystickThreshold && player1Y != GRID_SIZE - 1)
        {
            updateCell(true, player1X, player1Y);
            player1Y += 1;
            if (renderPlayer2)
                sendCommand(MoveDown);
            joystickReset = false;
        }
        // Move left
        else if (Nunchuk.state.joy_x_axis < centre - joystickThreshold && player1X != 0)
        {
            updateCell(true, player1X, player1Y);
            player1X -= 1;
            if (renderPlayer2)
                sendCommand(MoveLeft);
            joystickReset = false;
        }
        // Move right
        else if (Nunchuk.state.joy_x_axis > centre + joystickThreshold && player1X != GRID_SIZE - 1)
        {
            updateCell(true, player1X, player1Y);
            player1X += 1;
            if (renderPlayer2)
                sendCommand(MoveRight);
            joystickReset = false;
        }

        if (!joystickReset)
        {
            updatePosition();
            updateCell(true);
        }
    }
}

uint8_t countAdjacentTreasures(uint8_t gridX, uint8_t gridY)
{
    uint8_t count = 0;

    // Walk through all adjacent cells
    for (int8_t offsetX = -1; offsetX <= 1; offsetX++)
    {
        for (int8_t offsetY = -1; offsetY <= 1; offsetY++)
        {
            // calculate the coordinates of the neighbor cell
            uint8_t neighborX = gridX + offsetX;
            uint8_t neighborY = gridY + offsetY;

            // Check if the neighbor cell is within the grid
            if (neighborX >= 0 && neighborX < GRID_SIZE &&
                neighborY >= 0 && neighborY < GRID_SIZE)
            {
                // Only count Treasures
                if (grid[neighborY][neighborX] == 1)
                {
                    count++;
                }
            }
        }
    }

    return count; // return all adjacent Treasures
}

// Function to generate treasures
void generateTreasures()
{
    for (int row = 0; row < GRID_SIZE; row++)
    {
        for (int col = 0; col < GRID_SIZE; col++)
        {
            grid[row][col] = 0;         // no Treasure
            revealed[row][col] = false; // row and column are not revealed
        }
    }

    int TreasuresPlaced = 0;
    while (TreasuresPlaced < TREASURE_COUNT)
    {
        int randomRow = random(0, GRID_SIZE);
        int randomCol = random(0, GRID_SIZE);

        if (grid[randomRow][randomCol] == 0)
        {
            grid[randomRow][randomCol] = 1; // Place Treasure
            TreasuresPlaced++;
        }
    }
}

// Function to dig a cell
void digAction(bool isPlayer1)
{
    uint8_t x = (isPlayer1 ? player1X : player2X);
    uint8_t y = (isPlayer1 ? player1Y : player2Y);

    // Check if within the grid and not revealed yet
    if (x >= 0 && x < GRID_SIZE && y >= 0 && y < GRID_SIZE)
    {
        if (revealed[y][x])
            return;

        revealed[y][x] = true;

        // Reveal the mine or the number of adjacent mines
        if (grid[y][x] == 1)
        {
            // Increment score when the player successfully digs a safe cell
            if (isPlayer1)
            {
                player1Score++;
                isTreasure = true;
            }
            else
            {
                player2Score++;
            }

            updateScore();
        }

        updateCell(isPlayer1);
    }
}

void displayScoreboard()
{
    // Display the scoreboard title and static information (this only needs to be done once)
    tft.setCursor(10, 45); // Position for the "Scoreboard" title
    tft.setTextSize(1);
    tft.print("Scoreboard");

    tft.setCursor(10, 115); // Position for "Current"
    tft.print("Current ");
    tft.setCursor(10, 135); // Position for "Position"
    tft.print("Position");

    updateScore();
    updatePosition();
    updateTimer();
}

void updatePosition()
{
    tft.setTextSize(1);

    // Clear previous grid position if it has changed
    tft.fillRect(15, 155, 25, 40, ILI9341_WHITE); // Clear the "X:" and "Y:" values

    // Update new X and Y grid positions
    tft.setCursor(10, 155);
    tft.print("X: ");
    tft.print(player1X + 1); // Add 1 to match the grid numbering (1-based)

    tft.setCursor(10, 175);
    tft.print("Y: ");
    tft.print(player1Y + 1); // Add 1 to match the grid numbering (1-based)
}

void updateTimer()
{
    tft.setTextSize(1);
    tft.fillRect(5, 220, 70, 40, ILI9341_WHITE); // Clear the "Timer" display
    tft.setCursor(5, 220);                       // Position for "Timer"
    tft.print("Timer: ");
    tft.print(gameTime); // Print the elapsed time
    tft.print("S");
}

void updateScore()
{
    tft.setTextSize(1);

    // Clear previous grid position if it has changed
    tft.fillRect(55, 55, 25, 40, ILI9341_WHITE); // Clear the player 1 and player 2 scores
    tft.setCursor(5, 65);                        // Position for "Player 1:"
    tft.print("Player 1: ");                     // Print the score of Player 1
    tft.print(player1Score);                     // Print the score of Player 1

    tft.setCursor(5, 85); // Position for "Player 2:"
    tft.print("Player 2: ");
    tft.print(player2Score); // Print the score of Player 2
}

bool isGameOver()
{
    if ((player1Score + player2Score) == TREASURE_COUNT)
        return true;

    return false;
}

void setPlayer1Coordinates(bool isLeader) {
    if (isLeader)
        player1X = player1Y = 0;
    else
        player1X = player1Y = GRID_SIZE-1;
}