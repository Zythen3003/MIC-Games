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

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

volatile uint16_t ticksSinceLastUpdate = 0; // used to refresh display at a consistent rate

volatile uint16_t toggleCount = 0; // keeps track of the number of toggles by IR emitter

volatile uint16_t dataToSend; // 16 bit integer sent via IR

bool isSending = false;


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

void updateDisplay(uint16_t *posXp, uint16_t *posYp)
{
  static uint16_t oldPosX = *posXp;
  static uint16_t oldPosY = *posYp;

  // Lees Nunchuk-joystick
  Nunchuk.getState(NUNCHUK_ADDRESS);

  *posXp += (Nunchuk.state.joy_x_axis - 127) / 32;
  *posYp += ((-Nunchuk.state.joy_y_axis + 255) - 127) / 32;

  // Beperk cursorpositie binnen schermgrenzen
  if (*posXp < RADIUS_PLAYER)
  {
    *posXp = RADIUS_PLAYER;
  }
  else if (*posXp > 320 - RADIUS_PLAYER - 1)
  {
    *posXp = 320 - RADIUS_PLAYER - 1;
  }

  if (*posYp < RADIUS_PLAYER)
  {
    *posYp = RADIUS_PLAYER;
  }
  else if (*posYp > 240 - RADIUS_PLAYER - 1)
  {
    *posYp = 240 - RADIUS_PLAYER - 1;
  }

  // Wis oude cursor door de rasterlijnen binnen de cirkel te hertekenen
  for (int y = oldPosY - RADIUS_PLAYER; y <= oldPosY + RADIUS_PLAYER; y++)
  {
    if (y % 20 == 0) // Controleer of y op een rasterlijn zit
    {
      tft.drawLine(0, y, ILI9341_TFTWIDTH, y, ILI9341_BLUE);
    }
  }
  for (int x = oldPosX - RADIUS_PLAYER; x <= oldPosX + RADIUS_PLAYER; x++)
  {
    if (x % 20 == 0) // Controleer of x op een rasterlijn zit
    {
      tft.drawLine(x, 0, x, ILI9341_TFTHEIGHT, ILI9341_BLUE);
    }
  }

  // Teken nieuwe cursor
  tft.fillCircle(*posXp, *posYp, RADIUS_PLAYER, ILI9341_BLUE);

  // Update oude positie
  oldPosX = *posXp;
  oldPosY = *posYp;
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

void setup(void)
{
  timerSetup();
  IRSetup();
  sei();
  tft.begin();
}

void SetupGrid(void)
{
  tft.setRotation(1);
  tft.fillScreen(ILI9341_WHITE); // Maak het scherm wit
  tft.setTextColor(ILI9341_BLACK);
  tft.setTextSize(2);

  // Instellingen voor het raster
  int cellSize = 20; // Grootte van een cel (in pixels)
  int screenWidth = 320; // Breedte van het scherm (pas aan je scherm aan)
  int screenHeight = 240; // Hoogte van het scherm (pas aan je scherm aan)
  int x, y;

  // Horizontale lijnen
  for (y = 0; y <= screenHeight; y += cellSize)
  {
    tft.drawLine(0, y, screenWidth, y, ILI9341_BLUE);
  }

  // Verticale lijnen
  for (x = 0; x <= screenWidth; x += cellSize)
  {
    tft.drawLine(x, 0, x, screenHeight, ILI9341_BLUE);
  }

  // Optioneel: Cursor plaatsen voor tekst
  tft.setCursor(10, 10);
  tft.print("Grid klaar!");
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
  SetupGrid();

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
    if (ticksSinceLastUpdate > 380) // 100FPS
    {
        // tft.println("dsajdslk");
      updateDisplay(posXp, posYp);
      ticksSinceLastUpdate = 0;
    }

    if (Nunchuk.state.c_button && !isSending) {
		  sendBits(0b0000000000000000);
    } 

    if (Nunchuk.state.z_button && !isSending) {
      sendBits(0b0000000000000001);
    }

    if (Nunchuk.state.c_button || Nunchuk.state.z_button) {
      Serial.println(isSending);
    }
  }
  return 0;
}