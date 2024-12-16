#include "menu.h"
#include "Nunchuk.h"
#include "Adafruit_ILI9341.h"
#include <HardwareSerial.h>
#include <GameMechanics.h>

#define BAUDRATE 9600
#define PCF8574A_ADDR 0x21        // I2C address of the PCF8574A

// Global variables 
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
volatile uint16_t ticksSinceLastUpdate;
bool gameStarted = false; // To track if the game has started
Menu menu(&tft);          // Initialize the menu with the display

volatile uint32_t lastDigTime = 0;  // Track the last dig time
uint32_t digCooldown = 35000; // 3 seconds cooldown

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
  lastDigTime++;
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

// Toevoegen van een functie om de cooldown weer te geven op de 7-segment display
void displayCooldown(uint32_t remainingTime) {
    // Zet de display op de resterende tijd (bijv. seconden)
    int seconds = remainingTime / 5000;  // Omrekeningen naar seconden
    if (seconds < 0 || seconds > 7) return;  // Zorg ervoor dat de waarde binnen 0-7 blijft
    Wire.beginTransmission(PCF8574A_ADDR);
    Wire.write(~segmentMap[seconds]); // Zet de segmenten voor de resterende tijd
    Wire.endTransmission();
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

        // Display the player's grid position and scoreboard
        displayScoreboard(posX, posY);
 
        if (ticksSinceLastUpdate > 380) // 100FPS
        {
            updateDisplay(posXp, posYp);
            ticksSinceLastUpdate = 0;
        }
        // Toon de cooldown op de 7-segment display
        displayCooldown(digCooldown - lastDigTime); // Toon de resterende tijd in milliseconden

        // Check for Nunchuk button presses
        if (Nunchuk.state.c_button || Nunchuk.state.z_button) {
            // Display the player's grid position when any button is pressed
            displayScoreboard(posX, posY);
        }

        if (Nunchuk.state.z_button && lastDigTime >= digCooldown) {
            digAction(*posXp, *posYp);
            displayScoreboard(posX, posY);
            lastDigTime = 0;  // Reset last dig time after action
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
