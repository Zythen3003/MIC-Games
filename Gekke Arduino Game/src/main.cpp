#include "menu.h"
#include "Nunchuk.h"
#include "Adafruit_ILI9341.h"
#include <HardwareSerial.h>
#include <GameMechanics.h>
#include "buzzer.h" // Include the buzzer header

#define BAUDRATE 9600
#define PCF8574A_ADDR 0x21 // I2C address of the PCF8574A
#define DIG_COOLDOWN 35000
void playNonBlockingCorrectSound(Buzzer &myBuzzer);
// Global variables
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);
Adafruit_FT6206 ts = Adafruit_FT6206();
volatile uint16_t ticksSinceLastUpdate;
bool gameStarted = false; // To track if the game has started
Menu menu(&tft);          // Initialize the menu with the display
Buzzer myBuzzer;          // Create an instance of the Buzzer class

volatile uint16_t lastDigTime = 0; // Track the last dig time

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
  if (lastDigTime < DIG_COOLDOWN)
  {
    lastDigTime++;
  }
}

void playCorrectSound(Buzzer &myBuzzer)
{
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
  tft.begin();        // Initialize the display here
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
  // myBuzzer.testBuzzer(); // Test the buzzer

}

// Display the cooldown for the digAction on the 7-segment display
void displayCooldown(uint16_t remainingTime)
{
  // Convert the remaining time to segments
  int segments = remainingTime / 5000;
  if (segments < 0 || segments > 7)
    return; // Only display 0-7 on the 7-segment display
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

    if (!gameStarted)
    {
      // Handle menu input until a game mode is selected
      menu.handleMenuInput();

      if (ts.touched())
      {
        // Retrieve touch data
        TS_Point tPoint = ts.getPoint();

        menu.handleTouchInput(tPoint);
      }
    }
    else
    {
      movePlayer();

      if (ticksSinceLastUpdate > 380) // 100FPS
      {
        doGameLoop();
        ticksSinceLastUpdate = 0;
      }

      displayCooldown(DIG_COOLDOWN - lastDigTime);

      myBuzzer.update(); // Update the buzzer state

      if (Nunchuk.state.z_button && lastDigTime >= DIG_COOLDOWN)
      {

        digAction(true);
        updateScore();

        if (isTreasure)
          {
            playNonBlockingCorrectSound(myBuzzer); // Non-blocking sound playback
            isTreasure = false;                    // Reset the isTreasure flag
          }
        lastDigTime = 0;    // Reset last dig time after action
      }

      if (isGameOver())
      {
        tft.fillScreen(ILI9341_DARKGREEN);
        myBuzzer.update(); // Continuously update the buzzer state
        menu.displayEndGameMessage(); // Call the member function to display the end game message
        // Wait until a button is pressed to proceed
        while (true)
        {
          Nunchuk.getState(NUNCHUK_ADDRESS); // Update the Nunchuk state
          if (Nunchuk.state.c_button || Nunchuk.state.z_button)
          {
            break; // Exit the loop when a button is pressed
          }
        }
        tft.fillScreen(ILI9341_DARKGREEN);
        myBuzzer.update();   // Update the buzzer state
        menu.drawMenu();     // Return to the main menu
        gameStarted = false; // Stop the game
      }
    }
  }

  return 0;
}


void playNonBlockingCorrectSound(Buzzer &myBuzzer) { // Plays the melody non-blocking.
  static int soundStep = 0; // Tracks the current tone in the sequence.
  static unsigned long toneStartTick = 0; // Stores the starting time of the current tone.
  static bool isPlayingMelody = false; // Flag to indicate if the melody is currently playing.

  // Define the tones with their frequencies and durations.
  const int tones[][2] = {
    {59, 10}, 
    {324, 15},
    {440, 15},
    {523, 15},
    {625, 15}
  };

  // If the melody is not currently playing
  if (!isPlayingMelody) { 
    // Start playing the melody.
    isPlayingMelody = true;
    // Record the starting time of the current tone. 
    toneStartTick = TCNT1; 

    // Play the current tone. 
    myBuzzer.playTone(tones[soundStep][0], tones[soundStep][1]);

    // Move to the next tone in the sequence. 
    soundStep = (soundStep + 1) % 5; 
  }

  // Check if the current tone has finished playing.
  if ((TCNT1 - toneStartTick) >= tones[soundStep][1] && isPlayingMelody) { 
    // Stop playing the melody.
    isPlayingMelody = false; 
    // Record the starting time of the next tone. 
    toneStartTick = TCNT1; 
  }

  // Update the buzzer state.
  myBuzzer.update(); 
}