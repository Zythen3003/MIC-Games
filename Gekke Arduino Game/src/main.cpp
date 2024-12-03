#include <avr/io.h>
#include <avr/interrupt.h>
#include <Wire.h>
#include "Adafruit_ILI9341.h"
#include "Nunchuk.h"
#include <HardwareSerial.h>


#define TOGGLENUMBER 38 // number of times the IR emitter toggles to send one bit
#define TFT_DC 9
#define TFT_CS 10
#define BAUDRATE 9600
#define CHUNKSIZE 32
#define BUFFERLEN 256
#define NUNCHUK_ADDRESS 0x52
#define RADIUS_PLAYER 5
#define INITIALONEDURATION 684    // 9ms
#define INITIALZERODURATION 342   // 4.5ms
#define DATALENGTH 32             // total amount of bits sent, including logical inverse
#define ALLOWEDINITIALVARIANCE 32 // allowed variance for initial one and zero
#define GRID_COLUMNS 12 // Aantal kolommen in de grid
#define GRID_ROWS 12    // Aantal rijen in de grid
#define MINE_COUNT 10   // Aantal mijnen
#define BUFFER_SIZE (2 * RADIUS_PLAYER * 2 * RADIUS_PLAYER)

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

volatile uint16_t ticksSinceLastUpdate = 0; // used to refresh display at a consistent rate

volatile uint16_t toggleCount = 0; // keeps track of the number of toggles by IR emitter

volatile uint16_t dataToSend; // 16 bit integer sent via IR

bool isSending = false;

int grid[GRID_ROWS][GRID_COLUMNS]; // Grid om mijnen bij te houden
bool revealed[GRID_ROWS][GRID_COLUMNS]; // Houdt bij of een vakje is gegraven

/*
  used to track which bit should be sent,
  maximum value 32 unless sending new bits, in which case it is set to 64
*/
volatile uint8_t bitTurn;
void SetupGrid();

/*
  returns either 1 or 0 depending on bitTurn, if bitTurn is 16 or above it will send the inverse of the first 16 bits
*/
volatile bool getDigitToSend()
{
  if (bitTurn >= 16)
  {
    return !((dataToSend >> (15 - (bitTurn - 16))) % 2);
  }
  return ((dataToSend >> (15 - bitTurn)) % 2);
}

volatile void sendZero(void)
{
  PORTD &= ~(1 << PORTD6);
}

volatile void sendOne(void)
{
  PORTD ^= (1 << PORTD6);
}

volatile bool sendingIR = false;

volatile bool recievingIR = false;

ISR(INT0_vect)
{
  if (!sendingIR)
  {
    recievingIR = true;
  }
}

void sendIR(void)
{
  if (bitTurn < DATALENGTH || bitTurn == 64)
  {
    if (bitTurn == 64) // if communication start
    {
      if (toggleCount < INITIALONEDURATION) // send 1 for 9ms
      {
        sendOne();
      }
      else if (toggleCount < INITIALONEDURATION + INITIALZERODURATION) // then send 0 for 4.5ms
      {
        sendZero();
      }
      else // then start sending data
      {
        bitTurn = 0;
        toggleCount = 0;
      }
    }
    else
    {
      if (toggleCount >= TOGGLENUMBER) // reset togglecount and go to next bit if togglecount reaches TOGGLENUMBER
      {
        toggleCount = 0;
        bitTurn++;
      }
      if (getDigitToSend())
      {
        sendOne();
      }
      else
      {
        sendZero();
      }
    }
    toggleCount++;
  }
  else
  { // When done sending
    sendingIR = false;
  }
}

/*
  Returns status of IR reciever, inverted to correspond with sent bits
*/
bool getRecieverStatus(void)
{
  return !((PIND >> PIND2) % 2);
}

uint16_t readCount = 0; // number of times certain things have been read, used in different ways in recieveIR()

uint16_t cursorBuffer[BUFFER_SIZE]; // Buffer voor achtergrond onder de cursor


enum recieveStatus
{
  initialOne,
  initialZero,
  dataBits,
  inverseBits
};

recieveStatus currentRecieveStatus = initialOne; // used to decern the current status of recieveIR()
uint16_t recievedBits = 0;

void resetRecieveIR(void) // resets all values needed in recieveIR to their starting values
{
  // Serial.println("IR Reset");
  isSending = false;
  recievingIR = false;
  currentRecieveStatus = initialOne;
  readCount = 0;
}

void recieveIR(void)
{
  static bool previousValue;
  static uint8_t bitCount; // used to track the number of bits read so far 
  static uint16_t currentBits; // temporary variable to keep track of bits

  switch (currentRecieveStatus)
  {
  case initialOne: // checks for 9ms 1
    if (readCount < INITIALONEDURATION - ALLOWEDINITIALVARIANCE)
    {
      if (getRecieverStatus())
      {
        readCount++;
      }
      else
      {
        resetRecieveIR();
      }
    }
    else
    {
      if (readCount > INITIALONEDURATION)
      {
        resetRecieveIR();
      }
      else if (!getRecieverStatus())
      {
        currentRecieveStatus = initialZero;
        readCount = 0;
      }
    }
    break;
  case initialZero: // checks for 4.5ms 0 
    if (!(readCount < INITIALZERODURATION - ALLOWEDINITIALVARIANCE))
    {
      if (readCount > INITIALZERODURATION || getRecieverStatus())
      {
        currentRecieveStatus = dataBits;
        readCount = 0;
        bitCount = 0;
        currentBits = 0;
      }
    }
    else
    {
      if (getRecieverStatus())
      {
        resetRecieveIR();
      }
    }
    readCount++;
    break;
  case dataBits: // reads data bits
    if (readCount == TOGGLENUMBER / 2)
    {
      currentBits = (currentBits << 1);
      currentBits |= previousValue;
      previousValue = getRecieverStatus();
    }
    else if (readCount == TOGGLENUMBER)
    {
      readCount = 0;
      bitCount++;
    }
    readCount++;
    if (bitCount > 16)
    {
      currentRecieveStatus = inverseBits;
      readCount = 0;
      bitCount = 0;
    }
    break;
  case inverseBits: // currently only used for resetting and setting recievedBits, might be used to check inverse later
    recievedBits = currentBits;
    
    Serial.print(F("IR Data Received: 0b"));
    Serial.println(recievedBits, BIN);
    Serial.print(F("suppose to be Data Received: 0x"));
    Serial.println(0b1010101010101010, BIN); // Print received data in hexadecimal format

    if (recievedBits == 0b0000000000000000)
      tft.fillScreen(ILI9341_MAGENTA);
    if (recievedBits == 0b0000000000000001)
      tft.fillScreen(ILI9341_RED);
    
    resetRecieveIR();
    break;
  }
}

int player1Score = 0; // Houd de score van Player 1 bij

int countAdjacentMines(int gridX, int gridY) {
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

void restoreOldCursorPosition(uint16_t oldX, uint16_t oldY) {
    // Bereken de gridpositie
    int gridX = (oldX - (4 * 20)) / 20;
    int gridY = oldY / 20;

    // Controleer of de positie binnen het speelveld valt
    if (gridX >= 0 && gridX < GRID_COLUMNS && gridY >= 0 && gridY < GRID_ROWS) {
        int cellX = 4 * 20 + gridX * 20;
        int cellY = gridY * 20;

        // Herstel de achtergrondkleur van de cel
        tft.fillRect(cellX + 1, cellY + 1, 18, 18, ILI9341_WHITE);

        // Herstel gridlijnen
        tft.drawLine(cellX, cellY, cellX + 20, cellY, ILI9341_BLACK); // Bovenste lijn
        tft.drawLine(cellX, cellY, cellX, cellY + 20, ILI9341_BLACK); // Linker lijn

        // Als het vakje is gegraven, herstel het nummer of de mijn
        if (revealed[gridY][gridX]) {
            if (grid[gridY][gridX] == 1) {
                // Teken mijn
                tft.fillRect(cellX + 5, cellY + 5, 10, 10, ILI9341_BLACK);
            } else {
                // Teken het nummer van omliggende mijnen
                int mineCount = countAdjacentMines(gridX, gridY);
                tft.setCursor(cellX + 6, cellY + 3);
                tft.setTextSize(1);
                tft.setTextColor(ILI9341_BLACK);
                tft.print(mineCount);
            }
        }
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
                            int mineCount = countAdjacentMines(gridX, gridY);
                            tft.setCursor(cellX + 6, cellY + 3);
                            tft.setTextSize(1);
                            tft.setTextColor(ILI9341_BLACK);
                            tft.print(mineCount);
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

// Functie om mijnen te genereren
void generateMines() {
    for (int row = 0; row < GRID_ROWS; row++) {
        for (int col = 0; col < GRID_COLUMNS; col++) {
            grid[row][col] = 0;      // Geen mijn
            revealed[row][col] = false; // Vakje niet gegraven
        }
    }

    int minesPlaced = 0;
    while (minesPlaced < MINE_COUNT) {
        int randomRow = random(0, GRID_ROWS);
        int randomCol = random(0, GRID_COLUMNS);

        if (grid[randomRow][randomCol] == 0) {
            grid[randomRow][randomCol] = 1; // Plaats mijn
            minesPlaced++;
        }
    }
}

// Functie om mijnen weer te geven
void drawMines() {
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
            // Er is een mijn - verhoog de score van Player 1
            player1Score++;

            // Wis de oude score door het cijfer te overschrijven met de achtergrondkleur
            tft.setCursor(85, 65); // Plaats direct achter "Player 1:"
            tft.setTextSize(1);
            tft.setTextColor(ILI9341_WHITE); // Witte achtergrondkleur
            tft.print(player1Score - 1); // Wis het oude cijfer

            // Teken de nieuwe score
            tft.setCursor(85, 65); // Plaats direct achter "Player 1:"
            tft.setTextSize(1);
            tft.setTextColor(ILI9341_BLACK); // Zwarte tekstkleur
            tft.print(player1Score); // Print de nieuwe score
        } else {
            // Geen mijn - graaf de cel vrij
            int digX = 4 * 20 + gridX * 20; // Pixelpositie voor de cel
            int digY = gridY * 20;
            tft.fillRect(digX + 1, digY + 1, 18, 18, ILI9341_WHITE); // Maak de cel wit

            // Tel omliggende mijnen
            int mineCount = countAdjacentMines(gridX, gridY);

            // Toon het aantal omliggende mijnen, inclusief 0
            tft.setCursor(digX + 6, digY + 3); // Centreer in de cel
            tft.setTextSize(1);
            tft.setTextColor(ILI9341_BLACK);
            tft.print(mineCount); // Print ook "0" als er geen omliggende mijnen zijn
        }
    }
}
void displayScoreboard(uint16_t posX, uint16_t posY) {
  // Static variables to remember the previous state
  static int lastGridX = -1;  // -1 to indicate initial value
  static int lastGridY = -1;  // -1 to indicate initial value

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
  tft.print("Player 1: 0");

  tft.setCursor(5, 85);  // Position for "Player 2:"
  tft.print("Player 2: 0");

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
}



ISR(TIMER0_COMPA_vect)
{
  if (sendingIR)
  {
    sendIR();
    // tft.setCursor(10,10);
    // tft.println("sending");
  }
  else if (bitTurn >= DATALENGTH) // makes sure the IR emitter is off while idle
  {
    // tft.setCursor(150, 10);
    // tft.print("recieving else");
    sendZero();
    if (recievingIR)
    {
      recieveIR();
    }
  }
  ticksSinceLastUpdate++;
}

void timerSetup(void)
{
  TIMSK0 |= (1 << OCIE0A); // enable comp match a interrupt
  TCCR0A |= (1 << WGM01);  // CTC-mode
  OCR0A = 52;              // Set compare match value for 38kHz
  TCCR0B |= (1 << CS01);   // Prescaler of 8
}


void IRSetup(void)
{
  EIMSK |= (1 << INT0);  // enable external INT0 interrupts
  EICRA |= (1 << ISC01); // interrupt on falling edge
  DDRD |= (1 << DDD6); // set IR pin output
}

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

void setup(void)
{
  timerSetup();
  IRSetup();
  sei();
  tft.begin();
    // Draw the grid only once at the start
  SetupGrid();
  generateMines();
   
}


/*
  Sets the variables needed to send bits, returns false if not possible (e.g. when sending or recieving IR)
*/

bool sendBits(uint16_t bitsToSend)
{
  isSending = true;
  if (!sendingIR && !recievingIR)
  {
    // tft.println("true");
    dataToSend = bitsToSend;
    bitTurn = 64;
    sendingIR = true;
    return true;
  }

    // tft.println("false");
  return false;
}

int main(void)
{
  setup();

  Serial.begin(9600);

	// join I2C bus as master
	Wire.begin();

	Nunchuk.begin(NUNCHUK_ADDRESS);

  uint16_t posX = ILI9341_TFTWIDTH / 2;
  uint16_t posY = ILI9341_TFTHEIGHT / 2;
  uint16_t *posXp = &posX;
  uint16_t *posYp = &posY;


  tft.fillCircle(posX, posY, RADIUS_PLAYER, ILI9341_BLUE);
  // sendBits(0b0100010110101010);

  while (1)
  {
    // Display the player's grid position and scoreboard
    displayScoreboard(posX, posY);

    if (ticksSinceLastUpdate > 380) // 100FPS
    {
        updateDisplay(posXp, posYp);
        ticksSinceLastUpdate = 0;
    }

    // Check for Nunchuk button presses
    if (Nunchuk.state.c_button || Nunchuk.state.z_button) {
        // Display the player's grid position when any button is pressed
        displayScoreboard(posX, posY);
    }

    // Button presses that trigger sending bits (optional)
    if (Nunchuk.state.c_button && !isSending) {
        sendBits(0b0000000000000000);
    } 

    if (Nunchuk.state.z_button && !isSending) {
        sendBits(0b0000000000000001);
    }
    
    if (Nunchuk.state.z_button) {
            digAction(*posXp, *posYp);
   }

    // if (Nunchuk.state.c_button || Nunchuk.state.z_button) {
    //     Serial.println(isSending);
    // }
}
  return 0;
}
