#include <Arduino.h>
#include "Infrarood.h"

void setup() {
    initInfrarood(); // Initialiseer de infraroodfunctionaliteit
    Serial.begin(9600); // Start seriële communicatie
    sei(); // Globale interrupts aan
}

int main(void) {
    setup();
    while (1) {
        // Verwerk ontvangen pulsen
        processReceivedPulses();

        // Debugging of andere functionaliteit kan hier worden toegevoegd
    }
    return 0;
}
