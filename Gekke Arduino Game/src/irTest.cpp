// #include <avr/io.h>
// #include <avr/interrupt.h>
// #include <Wire.h>

// #define ON 0
// #define OFF 0b11111111
// #define SEVENSEGMENTADDR 0x21
// #define TOGGLENUMBER 10

// volatile bool segmentUpdateStatus = 0;
//   static bool currentStatus = 0;

// ISR(INT0_vect)
// {
//   segmentUpdateStatus = !segmentUpdateStatus;
// }

// volatile int toggleCount = TOGGLENUMBER;

// ISR(TIMER0_COMPA_vect)
// {
//   if (toggleCount < TOGGLENUMBER)
//   {
//     PORTD ^= (1 << PORTD6);
//     toggleCount++;
//   }
// }

// volatile void sendSignal()
// {
//   if (toggleCount >= TOGGLENUMBER)
//   {
//     toggleCount = 0;
//   }
// }

// ISR(PCINT1_vect)
// {
//   sendSignal();
// }

// void toggleSegmentDisplay(void)
// {
//   Wire.beginTransmission(SEVENSEGMENTADDR);
//   if (currentStatus)
//   {
//     Wire.write(OFF);
//   }
//   else
//   {
//     Wire.write(ON);
//   }
//   currentStatus = !currentStatus;
//   Wire.endTransmission();
// }

// void timerSetup(void)
// {
//   TIMSK0 |= (1 << OCIE0A); // enable comp match a interrupt
//   TCCR0A |= (1 << WGM01);  // CTC-mode
//   OCR0A = 210;             // set TOP to 210
//   TCCR0B |= (1 << CS00);   // no prescaler
// }

// void buttonSetup(void)
// {
//   PORTC |= (1 << PORTC1);
//   PCICR |= (1 << PCIE1);
//   PCMSK1 |= (1 << PCINT9);
// }

// void IRSetup(void)
// {
//   EIMSK |= (1 << INT0);  // enable external INT0 interrupts
//   EICRA |= (1 << ISC01); // interrupt on falling edge
//   DDRD |= (1 << DDD6);   // set IR pin output
// }

// void segmentDisplaySetup(void)
// {
//   Wire.begin();
//   Wire.beginTransmission(SEVENSEGMENTADDR);
//   Wire.write(OFF);
//   Wire.endTransmission();
// }

// void setup(void)
// {
//   timerSetup();
//   buttonSetup();
//   IRSetup();
//   sei();
//   segmentDisplaySetup();
// }

// void updateSegmentDisplay(void)
// {
//   if (segmentUpdateStatus)
//   {
//     toggleSegmentDisplay();
//     segmentUpdateStatus = 0;
//   }
// }

// int main(void)
// {
//   setup();
//   while (1)
//   {
//     updateSegmentDisplay();
//   }
//   return 0;
// }