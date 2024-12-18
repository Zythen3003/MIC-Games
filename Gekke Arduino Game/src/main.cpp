#include <stdint.h>
#include "menu.h"
#include "Nunchuk.h"
#include "Adafruit_ILI9341.h"
#include <HardwareSerial.h>
#include <GameMechanics.h>
#include <Multiplayer.h>

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
bool gameStarted = false; // To track if the game has started
bool multiplayer = false;
Menu menu(&tft);          // Initialize the menu with the display

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
  tft.begin(); // Initialize the display here
  tft.setRotation(1); // Adjust screen orientation to landscape mode
  Serial.begin(9600);

  // Initialize the Nunchuk
  Wire.begin();
  Nunchuk.begin(NUNCHUK_ADDRESS);

  // Draw the initial menu
  menu.drawMenu();
  
}

int main(void)
{
    setup();

    uint16_t posX = ILI9341_TFTWIDTH / 2;
    uint16_t posY = ILI9341_TFTHEIGHT / 2;
    uint16_t *posXp = &posX;
    uint16_t *posYp = &posY;

    while (1)
    {
        if (!gameStarted) {
            // Handle menu input until a game mode is selected
            menu.handleMenuInput();
        } else {
            // The game logic begins once a mode is selected
            tft.fillCircle(posX, posY, RADIUS_PLAYER, ILI9341_BLACK);

            if (multiplayer) {
                tft.fillCircle(getPlayer2X(), getPlayer2Y(), RADIUS_PLAYER, ILI9341_RED);
            }

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
                digAction(*posXp, *posYp, true);
                displayScoreboard(posX, posY);
            }
            // Check if the game is over
            if (isGameOver()) {
                gameStarted = false; // Stop the game
                menu.displayEndGameMessage(); // Call the member function to display the end game message
                menu.drawMenu(); // Redraw the main menu after game over
            }
        }
    }
    
    return 0;
}
