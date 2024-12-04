#include <avr/io.h>
#include <avr/interrupt.h>
#include "Infrarood.H"

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