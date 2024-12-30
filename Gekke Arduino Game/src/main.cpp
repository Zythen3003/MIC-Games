#include "menu.h"
#include "Nunchuk.h"
#include "Adafruit_ILI9341.h"
#include <HardwareSerial.h>
#include <GameMechanics.h>
#include "buzzer.h" // Include the buzzer header

#define BAUDRATE 9600
#define PCF8574A_ADDR 0x21        // I2C address of the PCF8574A

// Global variables 
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
Adafruit_FT6206 ts = Adafruit_FT6206();
volatile uint16_t ticksSinceLastUpdate;
bool gameStarted = false; // To track if the game has started
Menu menu(&tft);          // Initialize the menu with the display
Buzzer myBuzzer; // Create an instance of the Buzzer class

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

void playCorrectSound(Buzzer& myBuzzer) {
    myBuzzer.playTone(59, 10);
    myBuzzer.playTone(440, 15); // A4 note for 200ms
    myBuzzer.playTone(523, 15); // C5 note for 200ms
    myBuzzer.playTone(659, 15); // E5 note for 200ms
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

  ts.begin();

  // Initialize the Nunchuk
  Wire.begin();
  Nunchuk.begin(NUNCHUK_ADDRESS);

    // Initialize IO Expander (assuming it's already configured correctly for controlling a 7-segment display)
  Wire.beginTransmission(PCF8574A_ADDR);
  Wire.write(0xFF); // Set IO pins as outputs (assuming 0x00 configures the pins correctly)
  Wire.endTransmission();

  // Draw the initial menu
  menu.drawMenu();
  //myBuzzer.testBuzzer(); // Test the buzzer
}

// Display the cooldown for the digAction on the 7-segment display
void displayCooldown(uint32_t remainingTime) {
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

  uint16_t posX = ILI9341_TFTWIDTH / 2;
  uint16_t posY = ILI9341_TFTHEIGHT / 2;
  uint16_t *posXp = &posX;
  uint16_t *posYp = &posY;

while (1)
{
  
    if (!gameStarted) {
        // Handle menu input until a game mode is selected
        menu.handleMenuInput();

        if (ts.touched()) {
            // Retrieve touch data
            TS_Point tPoint = ts.getPoint();

            menu.handleTouchInput(tPoint);
        }
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
        // Display the cooldown for the digAction on the 7-segment display
        displayCooldown(digCooldown - lastDigTime); // Display the remaining time on the 7-segment display
        myBuzzer.update(); // Update the buzzer state
        // Check for Nunchuk button presses
        if (Nunchuk.state.c_button || Nunchuk.state.z_button) {
            // Display the player's grid position when any button is pressed
            displayScoreboard(posX, posY);
        }

        if (Nunchuk.state.z_button && lastDigTime >= digCooldown) {
        
            digAction(*posXp, *posYp);
            displayScoreboard(posX, posY);
            if(isTreasure){ playCorrectSound(myBuzzer);}
            isTreasure = false; // Reset the isTreasure flag
            lastDigTime = 0;  // Reset last dig time after action
        }
            
      if (isGameOver()) {
        menu.displayEndGameMessage(); // Call the member function to display the end game message
        // Wait until a button is pressed to proceed
        while (true) {
            Nunchuk.getState(NUNCHUK_ADDRESS); // Update the Nunchuk state
            if (Nunchuk.state.c_button || Nunchuk.state.z_button) {
                break; // Exit the loop when a button is pressed
            }
        }
        menu.drawMenu(); // Return to the main menu
        gameStarted = false; // Stop the game
      }
    }
}
return 0;
}
