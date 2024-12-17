#include <Arduino.h>
#include "Infrarood.h"

void setup() {
    Serial.begin(9600);
    initInfrarood();
    sei(); // Zet globale interrupts aan
}

int main(void) {
    setup();
    
    uint8_t dataToSend = 0b10101010; // Voorbeelddata
    sendDataByte(dataToSend); // Verstuur de data maar één keer
    
    while (1) {
        processReceivedPulses(); // Verwerk ontvangen signalen

        if (newDataAvailable) { // Controleer of nieuwe data beschikbaar is
            Serial.print("Ontvangen byte: ");
            Serial.println(tempByte, BIN);

            newDataAvailable = false; // Reset de vlag
            tempByte = 0;             // Reset tempByte
        }
    }
    return 0;
}
