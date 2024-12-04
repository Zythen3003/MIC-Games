#include <avr/io.h>
#include <avr/interrupt.h>
#include <Wire.h>
#include "Adafruit_ILI9341.h"
#include "Nunchuk.h"
#include "Grid.h"
#include <HardwareSerial.h>

#define TFT_DC 9
#define TFT_CS 10
#define BAUDRATE 9600
#define CHUNKSIZE 32
#define BUFFERLEN 256

#define BUFFER_SIZE (2 * RADIUS_PLAYER * 2 * RADIUS_PLAYER)

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
int grid[GRID_ROWS][GRID_COLUMNS]; // Grid om mijnen bij te houden
bool revealed[GRID_ROWS][GRID_COLUMNS]; // Houdt bij of een vakje is gegraven

volatile uint16_t ticksSinceLastUpdate = 0; // used to refresh display at a consistent rate

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
    // Draw the grid only once at the start
  SetupGrid();

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

    while (1)
    {
        // Display the player's grid position and scoreboard
        if (ticksSinceLastUpdate > 380) // 100FPS
        {
            updateDisplay(posXp, posYp);
            ticksSinceLastUpdate = 0;
        }
    }

    return 0;
}
