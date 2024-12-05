#include <avr/io.h>
#include <avr/interrupt.h>
#include <Wire.h>
#include "Adafruit_ILI9341.h"
#include "Nunchuk.h"
#include <HardwareSerial.h>
#include <GameMechanics.h>

#define TOGGLENUMBER 38 // number of times the IR emitter toggles to send one bit
#define BAUDRATE 9600
#define CHUNKSIZE 32
#define BUFFERLEN 256
#define INITIALONEDURATION 684    // 9ms
#define INITIALZERODURATION 342   // 4.5ms
#define DATALENGTH 32             // total amount of bits sent, including logical inverse
#define ALLOWEDINITIALVARIANCE 32 // allowed variance for initial one and zero

// Global variables 
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
volatile uint16_t ticksSinceLastUpdate;

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
  tft.begin(); // Initialize the display here (if not already done in SetupGrid)
    // Draw the grid only once at the start
  SetupGrid();
  generateTreasures();
}

int main(void)
{
    Serial.begin(9600);
    setup();

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
    
    if (Nunchuk.state.z_button) {
      digAction(*posXp, *posYp);
      displayScoreboard(posX, posY);
   }
}
  return 0;
}
