#include "InfraRood.h"
#include <util/delay.h>
#include <avr/io.h>

InfraRood::InfraRood()
    : bufferHead(0), bufferTail(0) {}

void InfraRood::begin() {
    // Timer 1 configureren
    TCCR1A = 0; // Normale modus
    TCCR1B = (1 << ICES1) | (1 << CS11); // Input capture op stijgende flank, prescaler 8
    TIMSK1 = (1 << ICIE1) | (1 << TOIE1); // Enable Input Capture en Overflow interrupt
}

void InfraRood::send(uint8_t data) {
    for (uint8_t i = 0; i < 8; i++) {
        bool bit = (data >> (7 - i)) & 0x01; // MSB eerst
        if (bit) {
            // Zet pin hoog voor een "1" puls
            PORTB |= (1 << PB1); // Veronderstelt dat de IR-LED op PB1 zit
            _delay_us(560);     // Typische "1" pulsduur
            PORTB &= ~(1 << PB1); // Zet pin laag
            _delay_us(1690);    // Rusttijd na "1" puls
        } else {
            // Zet pin hoog voor een "0" puls
            PORTB |= (1 << PB1);
            _delay_us(560);     // Typische "0" pulsduur
            PORTB &= ~(1 << PB1);
            _delay_us(560);     // Rusttijd na "0" puls
        }
    }
}

uint8_t InfraRood::receive() {
    if (!available()) return 0;

    uint8_t data = receiveBuffer[bufferTail];
    bufferTail = (bufferTail + 1) % 8;
    return data;
}

bool InfraRood::available() {
    return bufferHead != bufferTail;
}

void InfraRood::processISR() {
    static uint16_t lastCapture = 0;
    uint16_t currentCapture = ICR1;
    uint16_t pulseWidth = currentCapture - lastCapture;
    lastCapture = currentCapture;

    // Decodeer pulslengte
    if (pulseWidth > 500 && pulseWidth < 700) {
        // Voeg logische 1 toe
    } else if (pulseWidth > 200 && pulseWidth < 400) {
        // Voeg logische 0 toe
    }
}

void InfraRood::sendPulse(bool state, uint16_t duration) {
    if (state)
        PORTB |= (1 << PB1);
    else
        PORTB &= ~(1 << PB1);

    _delay_us(duration);
}
