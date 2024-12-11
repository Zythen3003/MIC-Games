#include "Infrarood.h"
#include <avr/io.h>
#include <avr/interrupt.h>

#define SECOND 15624            // Voor 1 seconde interval (bij 16 MHz en prescaler 1024)
#define BURST_1_DURATION 38     // Aantal cycli voor een 1 (1000 µs bij 38 kHz)
#define BURST_0_DURATION 19     // Aantal cycli voor een 0 (500 µs bij 38 kHz)
#define PAUSE_DURATION 19       // Aantal cycli voor 500 µs pauze
#define BUFFER_SIZE 16          // Maximaal aantal pulsen in buffer

volatile uint16_t pulseBuffer[BUFFER_SIZE];
volatile uint8_t bufferHead = 0;
volatile uint8_t bufferTail = 0;
volatile bool bufferOverflow = false;
volatile bool newPulseAvailable = false;

volatile uint8_t burstCounter = 0;
volatile bool sending = false;
volatile uint8_t currentBit = 0;
volatile uint8_t dataByte = 0;
volatile uint8_t bitIndex = 0;
volatile bool isPausing = false;

void timer0Setup(void) {
    TCCR0A = 0;
    TCCR0B = 0;
    TCCR0A |= (1 << WGM01);
    OCR0A = 52;
    TIMSK0 |= (1 << OCIE0A);
    TCCR0B |= (1 << CS01);
}

void timer1Setup(void) {
    TCCR1A = 0;
    TCCR1B = (1 << CS12) | (1 << CS10) | (1 << WGM12);
    OCR1A = SECOND;
    TIMSK1 = (1 << OCIE1A);
}

void setupInterrupt0(void) {
    EICRA |= (1 << ISC01);
    EIMSK |= (1 << INT0);
    DDRD &= ~(1 << DDD2);
    PORTD |= (1 << PORTD2);
}

void sendNextBit() {
    if (bitIndex < 8) {
        if (!isPausing) {
            currentBit = (dataByte >> (7 - bitIndex)) & 0x01;
            burstCounter = (currentBit == 1) ? BURST_1_DURATION : BURST_0_DURATION;
            TCCR0A |= (1 << COM0A0);
            isPausing = true;
        } else {
            burstCounter = PAUSE_DURATION;
            isPausing = false;
            TCCR0A &= ~(1 << COM0A0);
            bitIndex++;
        }
    } else {
        sending = false;
    }
}

ISR(TIMER1_COMPA_vect) {
    if (!sending) {
        sending = true;
        bitIndex = 0;
        isPausing = false;
        sendNextBit();
    }
}

ISR(TIMER0_COMPA_vect) {
    if (burstCounter > 0) {
        burstCounter--;
        if (burstCounter == 0 && sending) {
            sendNextBit();
        }
    }
}

ISR(INT0_vect) {
    PORTB ^= (1 << PORTB5);
    
    static uint16_t lastFallingEdgeTime = 0;
    uint16_t currentTime = TCNT1;
    uint16_t pulseDuration = currentTime - lastFallingEdgeTime;
    lastFallingEdgeTime = currentTime;

    uint8_t nextHead = (bufferHead + 1) % BUFFER_SIZE;
    if (nextHead == bufferTail) {
        bufferOverflow = true;
    } else {
        pulseBuffer[bufferHead] = pulseDuration;
        bufferHead = nextHead;
        newPulseAvailable = true;
    }
}

void processPulse(uint16_t pulseDuration) {
    static uint8_t tempByte = 0;
    static uint8_t receivedBits = 0;

    Serial.print("Pulse breedte: ");
    Serial.println(pulseDuration);

    if (pulseDuration <= 19) {
        tempByte = (tempByte << 1) | 0;
        receivedBits++;
    } else if (pulseDuration >= 20) {
        tempByte = (tempByte << 1) | 1;
        receivedBits++;
    }

    if (receivedBits == 8) {
        Serial.print("Ontvangen byte: ");
        Serial.println(tempByte, BIN);
        tempByte = 0;
        receivedBits = 0;
    }
}

void processReceivedPulses() {
    if (newPulseAvailable) {
        cli();
        while (bufferTail != bufferHead) {
            uint16_t pulseDuration = pulseBuffer[bufferTail];
            bufferTail = (bufferTail + 1) % BUFFER_SIZE;
            sei();
            processPulse(pulseDuration);
            cli();
        }
        newPulseAvailable = false;
        sei();
    }

    if (bufferOverflow) {
        Serial.println("Buffer overflow!");
        bufferOverflow = false;
    }
}

void initInfrarood() {
    DDRD |= (1 << DDD6);
    DDRD &= ~(1 << DDD2);
    DDRB |= (1 << DDB5);       // Zet pin 13 (LED) als uitgang
    timer0Setup();
    timer1Setup();
    setupInterrupt0();
    dataByte = 0b10101010;
}
