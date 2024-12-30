#include "menu.h"
#include "Nunchuk.h"
#include "Adafruit_ILI9341.h"
#include <HardwareSerial.h>
#include <GameMechanics.h>

#define BAUDRATE 9600
#define PCF8574A_ADDR 0x21        // I2C address of the PCF8574A
#define DIG_COOLDOWN 35000

// Global variables 
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
volatile uint16_t ticksSinceLastUpdate;
bool gameStarted = false; // To track if the game has started
Menu menu(&tft);          // Initialize the menu with the display

volatile uint16_t lastDigTime = 0;  // Track the last dig time

// Segment mapping for digits 0-9 for 7-segment display (common cathode)
const uint8_t segmentMap[10] = {
    0b00111111, // 0
    0b00111111, // 1
    0b00011111, // 2
    0b00001111, // 3
    0b00000111, // 4
    0b00000011, // 5
    0b00000001, // 6
};

ISR(TIMER0_COMPA_vect)
{
  ticksSinceLastUpdate++;
  if (lastDigTime < DIG_COOLDOWN) {
    lastDigTime++;
  }
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

    // Initialize IO Expander (assuming it's already configured correctly for controlling a 7-segment display)
  Wire.beginTransmission(PCF8574A_ADDR);
  Wire.write(0xFF); // Set IO pins as outputs (assuming 0x00 configures the pins correctly)
  Wire.endTransmission();

  // Draw the initial menu
  menu.drawMenu();
}

// Display the cooldown for the digAction on the 7-segment display
void displayCooldown(uint16_t remainingTime) {
    // Convert the remaining time to segments
    int segments = remainingTime / 5000;
    if (segments < 0 || segments > 7) return;  // Only display 0-7 on the 7-segment display
    Wire.beginTransmission(PCF8574A_ADDR);
    Wire.write(~segmentMap[segments]); // Display the remaining time on the 7-segment display
    Wire.endTransmission();
}

int main(void)
{
    setup();

    Serial.println("Start Game");

    while (1)
    {
        if (!gameStarted) {
            // Handle menu input until a game mode is selected
            menu.handleMenuInput();
        } else {
            // Display the player's grid position and scoreboard
            movePlayer();

            if (ticksSinceLastUpdate > 380) // 100FPS
            {
                doGameLoop();
                ticksSinceLastUpdate = 0;
            }

            // Display the cooldown for the digAction on the 7-segment display
            displayCooldown(DIG_COOLDOWN - lastDigTime); // Display the remaining time on the 7-segment display

            if (Nunchuk.state.z_button && lastDigTime >= DIG_COOLDOWN) {
                digAction(true);
                displayScoreboard();
                lastDigTime = 0;  // Reset last dig time after action
            }
                
            // Check if the game is over
            if (isGameOver()) {
                gameStarted = false; // Stop the game
                menu.displayEndGameMessage(); // Call the member function to display the end game message
                menu.drawMenu(); // Redraw the main menu after game over
            }

            displayScoreboard();
        }
    }

    return 0;
}
