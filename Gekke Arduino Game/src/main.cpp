#include <avr/io.h>
#include <avr/interrupt.h>

#define IR_PIN 6       // D6 is ingesteld als IR-output
#define CPU_FREQUENCY 2000000  // 16 MHz, bijvoorbeeld voor een microcontroller zoals een Arduino

void delayMicroseconds(unsigned int microseconds);
void delayMilliseconds(unsigned int milliseconds);

// Functie voor een vertraging in microseconden
void delayMicroseconds(unsigned int microseconds) {
    unsigned int cycles = (CPU_FREQUENCY / 1000000) * microseconds / 3;  // Verdeel door 3 voor gemiddelde instructietijd
    for (volatile unsigned int i = 0; i < cycles; i++) {
        // Busy-wait: niets doen, gewoon wachten
    }
}

// Functie voor een vertraging in milliseconden
void delayMilliseconds(unsigned int milliseconds) {
    for (unsigned int i = 0; i < milliseconds; i++) {
        delayMicroseconds(1000);  // 1000 microseconden per milliseconde
    }
}

void timerSetup(void)
{
  TCCR0A = 0;                // Reset Timer0 Control Register A
  TCCR0B = 0;                // Reset Timer0 Control Register B
  TIMSK0 |= (1 << OCIE0A); // vergelijking aan voor interups
  TCCR0A |= (1 << WGM01);  // Zet timer 0 in ctc mode
  OCR0A = 52;              // Set compare match value for 38kHz
  TCCR0B |= (1 << CS01);   // Zorgt dat de prescaler in de timer op 8 staat zodat de frequentie van de timer 38khz wordt
}
//f output = f cpu / 2⋅prescaler⋅(OCR0A+1)
// 16.000.000 / 2 x 8 x (24+1) = 38.000 dus 38 khz

void IRSetup(void)
{
  EIMSK |= (1 << INT0);  // schakelt de interrupt op pin 2 in.
  EICRA |= (1 << ISC01); // stelt de interupt in bij verandering van signaal hoog naar laag.
  DDRD |= (1 << DDD6); // Output pin voor led
}

void SendOne(){
  TCCR0A |= (1 << COM0A0);  // Verbind Timer0 met de pin
  delayMicroseconds(1120);

  TCCR0A &= ~(1 << COM0A0); // Ontkoppel Timer0 van de pin
  delayMicroseconds(560);
  }

void SendZero(){
  TCCR0A |= (1 << COM0A0);  // Verbind Timer0 met de pin
  delayMicroseconds(560);

  TCCR0A &= ~(1 << COM0A0); // Ontkoppel Timer0 van de pin
  delayMicroseconds(560);}

void setup(void)
{
   timerSetup();
   IRSetup();
}

void loop()
{
  SendOne();
  SendOne();
  SendZero();
  SendOne();
  SendZero();
  SendZero();
  delayMilliseconds(1000);
}

int main(void) {
    setup();
    while (1) {
        loop();
    }
}