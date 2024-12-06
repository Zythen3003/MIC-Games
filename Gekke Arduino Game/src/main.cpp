#include <avr/io.h>
#include <avr/interrupt.h>
#include <Wire.h>
#include "Adafruit_ILI9341.h"
#include "Nunchuk.h"
#include "menu.h"
#include "Grid.h"
#include <HardwareSerial.h>

#define TFT_DC 9
#define TFT_CS 10
#define BAUDRATE 9600
#define CHUNKSIZE 32
#define BUFFERLEN 256

#define BUFFER_SIZE (2 * RADIUS_PLAYER * 2 * RADIUS_PLAYER)

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
int grid[GRID_ROWS][GRID_COLUMNS]; // Grid to track mines
bool revealed[GRID_ROWS][GRID_COLUMNS]; // Tracks whether a cell has been revealed
Menu menu(&tft); // Initialize the menu system

volatile uint16_t ticksSinceLastUpdate = 0; // Used to refresh display at a consistent rate
bool gameStarted = false;

ISR(TIMER0_COMPA_vect)
{
  ticksSinceLastUpdate++;
}

void timerSetup(void)
{
  TIMSK0 |= (1 << OCIE0A); // enable comp match a interrupt
  TCCR0A |= (1 << WGM01);  // CTC-mode
  OCR0A = 52;              // Set compare match value for 38kHz
  TCCR0B |= (1 << CS01);   // Prescaler of 8
}

void setup(void)
{
  timerSetup();
  sei();
  tft.begin();
  SetupGrid();
  // Draw the menu
  menu.drawMenu();
}


int main(void)
{
    setup();

    Serial.begin(9600);

    // Join I2C bus as master
    Wire.begin();

    Nunchuk.begin(NUNCHUK_ADDRESS);

    uint16_t posX = ILI9341_TFTWIDTH / 2;
    uint16_t posY = ILI9341_TFTHEIGHT / 2;
    uint16_t *posXp = &posX;
    uint16_t *posYp = &posY;

    while (1)
    {
        if (gameStarted) {
            // Game has started, so handle gameplay
            // This block will only execute after a valid mode selection

            // Display the player's grid position and scoreboard
            if (ticksSinceLastUpdate > 380) // 100FPS
            {
                updateDisplay(posXp, posYp);
                ticksSinceLastUpdate = 0;
            }

            // Handle the gameplay logic (e.g., player movement, grid updates, etc.)
            // Implement your game logic here

        } else {
            // If the game hasn't started, continue showing the menu
            menu.handleMenuInput();
        }
    }

    return 0;
}
