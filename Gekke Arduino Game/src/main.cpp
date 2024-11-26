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
  recievingIR = false;
  currentRecieveStatus = initialOne;
  readCount = 0;
}

/*
  Function used by TIMER0 to recieve IR
*/
void recieveIR(void)
{
    // tft.setCursor(10, 30);
    // tft.println("recieving");
  static bool previousValue;
  static uint8_t bitCount; // used to track the number of bits read so far 
  static uint16_t currentBits; // temporary variable to keep track of bits

  switch (currentRecieveStatus)
  {
  case initialOne: // checks for 9ms 1
    // tft.setCursor(10, 40);
    // tft.print("InitialOne recieving beginning");
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
    //   tft.fillRect(0, 40, 20, 25, ILI9341_WHITE);
    //   tft.println(readCount);
        Serial.print(readCount);
    }
    else
    {
      if (readCount > INITIALONEDURATION)
      {
        resetRecieveIR();
      }
      else if (!getRecieverStatus())
      {
        // tft.setCursor(10, 40);
        // tft.println("To initialZero");
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
        Serial.print('1');
      currentBits = (currentBits << 1);
      currentBits |= previousValue;
      previousValue = getRecieverStatus();
    }
    else if (readCount == TOGGLENUMBER)
    {
        Serial.print('0');
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
    // tft.setCursor(10, 60);
    // tft.println("complete");
    // tft.setCursor(10, 80);
    // tft.println(currentBits);
    Serial.print("recieve done ");
    Serial.println(currentBits);
    recievedBits = currentBits;
    resetRecieveIR();
    break;
  }
}

void updateDisplay(uint16_t *posXp, uint16_t *posYp)
{
  uint16_t oldPosX = *posXp;
  uint16_t oldPosY = *posYp;
  Nunchuk.getState(NUNCHUK_ADDRESS);
  *posXp += (Nunchuk.state.joy_x_axis - 127) / 32;
  *posYp += ((-Nunchuk.state.joy_y_axis + 255) - 127) / 32;

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

  tft.fillCircle(oldPosX, oldPosY, RADIUS_PLAYER, ILI9341_WHITE);
  tft.fillCircle(*posXp, *posYp, RADIUS_PLAYER, ILI9341_BLUE);
}

ISR(TIMER0_COMPA_vect)
{
  if (sendingIR)
  {
    sendIR();
    tft.setCursor(10,10);
    tft.println("sending");
  }
  else if (bitTurn >= DATALENGTH) // makes sure the IR emitter is off while idle
  {
    tft.setCursor(150, 10);
    tft.print("recieving else");
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
  OCR0A = 210;             // set TOP to 210
  TCCR0B |= (1 << CS00);   // no prescaler
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
  tft.setRotation(1);
}

/*
  Sets the variables needed to send bits, returns false if not possible (e.g. when sending or recieving IR)
*/
bool sendBits(uint16_t bitsToSend)
{
    tft.setCursor(100, 10);
  if (!sendingIR && !recievingIR)
  {
    tft.println("true");
    dataToSend = bitsToSend;
    bitTurn = 64;
    sendingIR = true;
    return true;
  }

    tft.println("false");
  return false;
}

int main(void)
{
  setup();

  Serial.begin(9600);

	// join I2C bus as master
	Wire.begin();

	Nunchuk.begin(NUNCHUK_ADDRESS);

	tft.setRotation(1);  // Adjust as needed for display orientation
    tft.setTextColor(ILI9341_BLACK);
    tft.setTextSize(2);
    tft.setCursor(10, 10);

  uint16_t posX = ILI9341_TFTWIDTH / 2;
  uint16_t posY = ILI9341_TFTHEIGHT / 2;
  uint16_t *posXp = &posX;
  uint16_t *posYp = &posY;
  tft.fillScreen(ILI9341_WHITE);

  tft.fillCircle(posX, posY, RADIUS_PLAYER, ILI9341_BLUE);
  sendBits(0b1000101110111001);

  while (1)
  {
    if (ticksSinceLastUpdate > 380) // 100FPS
    {
        // tft.println("dsajdslk");
      updateDisplay(posXp, posYp);
      ticksSinceLastUpdate = 0;
    }
  }
  return 0;
}