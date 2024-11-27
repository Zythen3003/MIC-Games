#include <avr/io.h>
#include <avr/interrupt.h>
#include <Wire.h>
#include "Adafruit_ILI9341.h"
#include "Nunchuk.h"
#include <HardwareSerial.h>

#define TOGGLENUMBER 38 // Number of toggles for IR emitter to send one bit
#define TFT_DC 9  // Pin for TFT Data/Command control
#define TFT_CS 10 // Pin for TFT Chip Select
#define BAUDRATE 9600  // Serial communication baud rate
#define CHUNKSIZE 32  // Size of data chunks
#define BUFFERLEN 256  // Size of buffer for data
#define NUNCHUK_ADDRESS 0x52  // I2C address for the Nunchuk
#define RADIUS_PLAYER 5  // Radius of the player's circle on the display
#define INITIALONEDURATION 684    // Duration for sending '1' (9ms)
#define INITIALZERODURATION 342   // Duration for sending '0' (4.5ms)
#define DATALENGTH 32             // Total amount of bits sent, including logical inverse
#define ALLOWEDINITIALVARIANCE 32 // Allowed variance for initial one and zero

// Adafruit TFT display object
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// Global variables
volatile uint16_t ticksSinceLastUpdate = 0; // Counter for timing to refresh the display
volatile uint16_t toggleCount = 0; // Counter for the number of IR emitter toggles
volatile uint16_t dataToSend; // Data to send via IR (16-bit integer)
bool isSending = false;   // Flag to track if we're sending data

/*
  used to track which bit should be sent,
  maximum value 32 unless sending new bits, in which case it is set to 64
*/
volatile uint8_t bitTurn;

/*
  returns either 1 or 0 depending on bitTurn, if bitTurn is 16 or above it will send the inverse of the first 16 bits
*/
volatile bool getDigitToSend()
{
  if (bitTurn >= 16)
  {
    // If bitTurn is 16 or higher, send the inverse of the first 16 bits
    return !((dataToSend >> (15 - (bitTurn - 16))) % 2);
  }
  return ((dataToSend >> (15 - bitTurn)) % 2); // Normal bit transmission
}

// Functions to send logic levels for IR communication
volatile void sendZero(void) {
    PORTD &= ~(1 << PORTD6);  // Set PORTD6 low for '0'
}

volatile void sendOne(void) {
    PORTD ^= (1 << PORTD6);  // Toggle PORTD6 for '1'
}

// ISR for external interrupt INT0 (IR receiver)
volatile bool sendingIR = false;
volatile bool recievingIR = false;
ISR(INT0_vect) {
    if (!sendingIR) {
        recievingIR = true;  // Start receiving if not sending
    }
}

// Main function to send IR data
void sendIR(void)
{
  if (bitTurn < DATALENGTH || bitTurn == 64){
  // Print the current bit turn and the toggle count
    Serial.print("Sending IR - bitTurn: ");
    Serial.print(bitTurn);
    Serial.print(" ToggleCount: ");
    Serial.println(toggleCount);    
    if (bitTurn == 64) // Initial communication setup
    {
      Serial.println("Starting communication setup (64 phase)");

      if (toggleCount < INITIALONEDURATION) // send 1 for 9ms
      {
        sendOne();
        Serial.println("Sending '1' for 9ms");
      }
      else if (toggleCount < INITIALONEDURATION + INITIALZERODURATION) // then send 0 for 4.5ms
      {
        sendZero();
        Serial.println("Sending '0' for 4.5ms");

      }
      else // Start sending actual data
      {
        bitTurn = 0;
        toggleCount = 0;
      }
    }
    else { // Send actual data
      if (toggleCount >= TOGGLENUMBER) // reset togglecount and go to next bit if togglecount reaches TOGGLENUMBER
      {
        toggleCount = 0;
        bitTurn++;
        Serial.print("Moving to next bit - bitTurn: ");
        Serial.println(bitTurn);
      }
      if (getDigitToSend())
      {
        sendOne(); // Send '1' for the current bit
        Serial.println("Sending '1'");
      }
      else
      {
        sendZero(); // Send '0' for the current bit
        Serial.println("Sending '0'");
      }
    }
    toggleCount++;
  }
  else{
    // When done sending data, stop sending
    Serial.println("Finished sending IR data");
    sendingIR = false;
  }
}

/*
  Function to get the receiver status (inverted to match sent data)
*/
bool getRecieverStatus(void)
{
  bool status = !((PIND >> PIND2) % 2); // Get receiver status (inverted)
  Serial.print("Receiver Status: ");
  Serial.println(status); // Print receiver status (0 or 1)
  return status;
}

// Variables for handling received IR data
uint16_t readCount = 0; // Counter for reads in the receiver
enum recieveStatus { 
    initialOne,   // Initial '1' detection (9ms)
    initialZero,  // Initial '0' detection (4.5ms)
    dataBits,     // Reading data bits
    inverseBits   // Inverse bits (for checking inverse data)
};

recieveStatus currentRecieveStatus = initialOne; // used to decern the current status of recieveIR()
uint16_t recievedBits = 0;  // Store received bits

void resetRecieveIR(void) // resets all values needed in recieveIR to their starting values
{
  // Serial.println("IR Reset");
  isSending = false;
  recievingIR = false;
  currentRecieveStatus = initialOne;
  readCount = 0;
}

// Function to receive IR data and process it
void recieveIR(void)
{
  static bool previousValue;
  static uint8_t bitCount; // used to track the number of bits read so far 
  static uint16_t currentBits; // temporary variable to keep track of bits

  switch (currentRecieveStatus)
  {
  case initialOne: // Detect '1' (9ms)
    Serial.print("State: initialOne - ");
    Serial.print("readCount: ");
    Serial.println(readCount);
    if (readCount < INITIALONEDURATION - ALLOWEDINITIALVARIANCE)
    {
      if (getRecieverStatus())
      {
        readCount++;
      }
      else
      {
        Serial.println("Error: No '1' detected. Resetting.");
        resetRecieveIR();// Reset if we didn't receive '1'
      }
    }
    else
    {
      if (readCount > INITIALONEDURATION)
      {
        Serial.println("Error: Timing out while detecting '1'. Resetting.");
        resetRecieveIR();
      }
      else if (!getRecieverStatus())
      {
        Serial.println("Detected initial '1' completed, moving to initialZero.");
        currentRecieveStatus = initialZero;
        readCount = 0;
      }
    }
    break;
  case initialZero: // checks for 4.5ms 0 
    Serial.print("State: initialZero - ");
    Serial.print("readCount: ");
    Serial.println(readCount);
    if (!(readCount < INITIALZERODURATION - ALLOWEDINITIALVARIANCE))
    {
      if (readCount > INITIALZERODURATION || getRecieverStatus())
      {
        currentRecieveStatus = dataBits;// Move to data bits if detected '0'
        readCount = 0;
        bitCount = 0;
        currentBits = 0;
      }
    }
    else
    {
      if (getRecieverStatus())
      {
        Serial.println("Error: Unexpected signal during '0' detection. Resetting.");

        resetRecieveIR();
      }
    }
    readCount++;
    break;
  case dataBits: // reads data bits
    Serial.print("State: dataBits - ");
    Serial.print("readCount: ");
    Serial.print(readCount);
    Serial.print(" - bitCount: ");
    Serial.println(bitCount);
    if (readCount == TOGGLENUMBER / 2)
    {
      currentBits = (currentBits << 1); // Shift left
      currentBits |= previousValue; // Add current value
      previousValue = getRecieverStatus();   // Store current status
      Serial.print("Shifted currentBits: ");
      Serial.println(currentBits, BIN);
      
    }
    else if (readCount == TOGGLENUMBER)
    {
      readCount = 0;
      bitCount++; // Move to the next bit after TOGGLENUMBER
      Serial.print("Moving to next bit - bitCount: ");
      Serial.println(bitCount);
    }
    readCount++;
     // Print the received data in binary format
    Serial.print("IR Data Received (binary): 0b");
    Serial.println(currentBits, BIN);


    // Print the received data in hexadecimal format
    Serial.print("IR Data Received (hex): 0x");
    Serial.println(currentBits, HEX);
    if (bitCount > 16)
    {
      currentRecieveStatus = inverseBits;  // If more than 16 bits received, check inverse
      readCount = 0;
      bitCount = 0;
    }
    break;
  case inverseBits: // currently only used for resetting and setting recievedBits, might be used to check inverse later
    recievedBits = currentBits;
    
    Serial.print(F("IR Data Received: 0b"));
    Serial.println(recievedBits, BIN);// Print received data in binary format
    Serial.print(F("suppose to be Data Received: 0x"));
    Serial.println(0b1010101010101010, BIN); // Expected pattern (for debugging)
    if (recievedBits == 0b0000000000000000)
      tft.fillScreen(ILI9341_MAGENTA); // Update TFT color if received data is 0
    if (recievedBits == 0b0000000000000001)
      tft.fillScreen(ILI9341_RED);  // Update TFT color if received data is 1
    
    resetRecieveIR(); // Reset after processing
    break;
  }
}

// Function to update the display with player position
void updateDisplay(uint16_t *posXp, uint16_t *posYp)
{
  uint16_t oldPosX = *posXp;
  uint16_t oldPosY = *posYp;
  
  // Get state from Nunchuk
  Nunchuk.getState(NUNCHUK_ADDRESS);

  // Update position based on Nunchuk joystick
  *posXp += (Nunchuk.state.joy_x_axis - 127) / 32;
  *posYp += ((-Nunchuk.state.joy_y_axis + 255) - 127) / 32;
  
  // Constrain player position to be within screen bounds
  if (*posXp < RADIUS_PLAYER)
  {
    *posXp = RADIUS_PLAYER;
  }
  else if (*posXp > ILI9341_TFTHEIGHT - RADIUS_PLAYER - 1)
  {
    *posXp = ILI9341_TFTHEIGHT - RADIUS_PLAYER - 1;
  }

  if (*posYp < RADIUS_PLAYER)
  {
    *posYp = RADIUS_PLAYER;
  }
  else if (*posYp > ILI9341_TFTWIDTH - RADIUS_PLAYER - 1)
  {
    *posYp = ILI9341_TFTWIDTH - RADIUS_PLAYER - 1;
  }

  // Update the TFT display to show the player's movement
  tft.fillCircle(oldPosX, oldPosY, RADIUS_PLAYER, ILI9341_WHITE);  // Erase old position
  tft.fillCircle(*posXp, *posYp, RADIUS_PLAYER, ILI9341_BLUE);  // Draw new position
}

// ISR for Timer0 Compare Match (38kHz frequency for IR)
ISR(TIMER0_COMPA_vect)
{
  if (sendingIR)
  {
    sendIR();  // Call sendIR if we are transmitting
    // tft.setCursor(10,10);
    // tft.println("sending");
  }
  else if (bitTurn >= DATALENGTH) // makes sure the IR emitter is off while idle
  {
    // tft.setCursor(150, 10);
    // tft.print("recieving else");
    sendZero();  // Keep sending '0' if we are idle
    if (recievingIR)
    {
      recieveIR();  // Call receiveIR if we are receiving data
    }
  }
  ticksSinceLastUpdate++;  // Increment tick count for screen refresh timing
}

// Timer setup for generating 38kHz frequency for IR
void timerSetup(void)
{
  TIMSK0 |= (1 << OCIE0A); // enable comp match a interrupt
  TCCR0A |= (1 << WGM01);  // CTC-mode
  OCR0A = 52;              // Set compare match value for 38kHz
  TCCR0B |= (1 << CS01);   // Prescaler of 8
}

// IR communication setup
void IRSetup(void)
{
  EIMSK |= (1 << INT0);  // enable external INT0 interrupts
  EICRA |= (1 << ISC01); // interrupt on falling edge
  DDRD |= (1 << DDD6); // set IR pin output
}

// Setup function called once at startup
void setup(void)
{
  timerSetup();
  IRSetup();
  sei();  // Enable global interrupts
  tft.begin();  // Initialize TFT display
  tft.setRotation(1); // Set TFT display rotation
}

/*
  Function to send bits via IR
*/
bool sendBits(uint16_t bitsToSend)
{
  isSending = true;
  if (!sendingIR && !recievingIR)
  {
    tft.println("true");
    dataToSend = bitsToSend;  // Set data to send
    bitTurn = 64;  // Start sending from the initial communication phase
    sendingIR = true;
    return true;  // Return true if sending is possible
  }

  tft.println("false");
  return false;  // Return false if we are already sending or receiving
}

int main(void)
{
  setup();  // Call setup to initialize everything
  Serial.begin(9600);  // Initialize serial communication
  Wire.begin();  // Start I2C communication
  Nunchuk.begin(NUNCHUK_ADDRESS);  // Initialize Nunchuk

  tft.setRotation(1);  // Set display rotation
  tft.setTextColor(ILI9341_BLACK);  // Set text color to black
  tft.setTextSize(2);  // Set text size
  tft.setCursor(10, 10);  // Set text cursor position

  uint16_t posX = ILI9341_TFTWIDTH / 2;  // Initialize player position X
  uint16_t posY = ILI9341_TFTHEIGHT / 2;  // Initialize player position Y
  uint16_t *posXp = &posX;
  uint16_t *posYp = &posY;

  tft.fillScreen(ILI9341_WHITE);  // Set the initial background to white
  tft.fillCircle(posX, posY, RADIUS_PLAYER, ILI9341_BLUE);  // Draw the player at the start

  while (1)
  {
    if (ticksSinceLastUpdate > 380) {  // Update display at 100 FPS
      updateDisplay(posXp, posYp);  // Update player position
      ticksSinceLastUpdate = 0;
    }

    // Check button presses on Nunchuk and send corresponding data
    if (Nunchuk.state.c_button && !isSending) {
      sendBits(0b0000000000000000);  // Send data when C button is pressed
    }

    if (Nunchuk.state.z_button && !isSending) {
      sendBits(0b0000000000000001);// Send data when Z button is pressed
    }

    if (Nunchuk.state.c_button || Nunchuk.state.z_button) {
      Serial.println(isSending); // Debug: Print if sending is in progress
    }
  }
  return 0;
}