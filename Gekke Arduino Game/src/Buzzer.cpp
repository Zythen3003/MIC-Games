#include "buzzer.h"

volatile bool Buzzer::isPlaying = false; // Define the static member

Buzzer::Buzzer() {
    // Set the buzzer pin as output using direct port manipulation
    DDRB |= (1 << BUZZER_PIN); // Assuming BUZZER_PIN is a pin on PORTB
    PORTB &= ~(1 << BUZZER_PIN); // Set the pin LOW
}

void Buzzer::setupTimer2(Frequency frequency) {
    Serial.print("Setting up Timer2 for frequency: ");
    Serial.println(frequency);

    TCCR2B = 0;
    TCCR2A = 0;

    if (frequency == 0) {
        Serial.println("Frequency is 0, returning");
        return;
    }

    TCCR2B |= (1 << CS21); // Prescaler 8
    TCCR2A |= (1 << WGM21); // CTC Mode

    OCR2A = (F_CPU / (8 * 2 * frequency)) - 1;
    Serial.print("OCR2A value: ");
    Serial.println(OCR2A);

    TIMSK2 |= (1 << OCIE2A);
    Serial.println("Timer2 interrupt enabled");
}

ISR(TIMER2_COMPA_vect) {
    if (Buzzer::isPlaying) {
        static bool buzzerState = false;
        // Toggle the buzzer pin using direct port manipulation
        if (buzzerState) {
            PORTB |= (1 << BUZZER_PIN); // Set the pin HIGH
        } else {
            PORTB &= ~(1 << BUZZER_PIN); // Set the pin LOW
        }
        buzzerState = !buzzerState;
    } else {
        // Ensure the buzzer is off using direct port manipulation
        PORTB &= ~(1 << BUZZER_PIN); // Set the pin LOW
        TIMSK2 &= ~(1 << OCIE2A);
        Serial.println("Timer2 interrupt disabled");
    }
}

void Buzzer::playTone(Frequency frequency, unsigned long duration) {
    Serial.print("Playing tone with frequency: ");
    Serial.println(frequency);
    if (frequency == 0) {
        return;
    }
    Buzzer::isPlaying = true;
    setupTimer2(frequency);
    _delay_ms(duration);
    noTone();
    Serial.println("Tone finished");
}

void Buzzer::noTone() {
    Serial.println("Stopping tone");
    Buzzer::isPlaying = false;
}