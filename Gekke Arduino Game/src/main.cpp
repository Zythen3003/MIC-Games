#include <Arduino.h>
#include "Infrarood.h"

void setup() {
    initInfrarood();
    Serial.begin(9600);
    sei(); // Globale interrupts aan
}

int main(void) {
    setup();
    
    while (1) {
        processReceivedPulses(); // Verwerk binnenkomende pulsen

        if (newDataAvailable) { // Controleer of nieuwe data beschikbaar is
            Serial.print("Ontvangen byte: ");
            Serial.println(tempByte, BIN);

            newDataAvailable = false; // Reset de vlag
            tempByte = 0;             // Reset tempByte
        }
    }
    return 0;
}
