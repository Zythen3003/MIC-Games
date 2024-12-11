#include "Infrarood.h"
#include <avr/io.h>
#include <avr/interrupt.h>

// Variabele beschikbaar maken voor main
uint32_t tempByte = 0;
bool newDataAvailable = false; // Nieuwe data beschikbaar

#define SECOND 15624
#define BURST_1_DURATION 38
#define BURST_0_DURATION 19
#define BURST_start_DURATION 342
#define BURST_pausestart_DURATION 171
#define PAUSE_DURATION 19
#define BUFFER_SIZE 32

volatile uint32_t pulseBuffer[BUFFER_SIZE];
volatile uint8_t bufferHead = 0;
volatile uint8_t bufferTail = 0;
volatile bool bufferOverflow = false;
volatile bool newPulseAvailable = false;

volatile uint16_t burstCounter = 0; // Maak burstCounter 16-bit
volatile bool sending = false;
volatile uint8_t currentBit = 0;
volatile uint32_t dataByte = 0;
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
    EICRA |= (1 << ISC01) | (1 << ISC00);
    EIMSK |= (1 << INT0);
    DDRD &= ~(1 << DDD2);
    PORTD |= (1 << PORTD2);
}

void sendNextBit() {
    static bool sendingStartBit = true; // Vlag om bij te houden of startbit wordt verzonden

    if (sendingStartBit) {
        if (!isPausing) {
            // Stuur het startbit (0)
            burstCounter = BURST_start_DURATION;
            TCCR0A |= (1 << COM0A0); // Activeer output voor burst
            isPausing = true;        // Start pauze na deze burst
        } else {
            burstCounter = BURST_pausestart_DURATION; // Pauze na het startbit
            isPausing = false;             // Reset pauze status
            TCCR0A &= ~(1 << COM0A0);      // Ontkoppel Timer0 van OC0A
            sendingStartBit = false;       // Startbit is verzonden, ga naar databytes
        }
    } else if (bitIndex < 32) {
        if (!isPausing) {
            // Stuur de bits van de databyte
            currentBit = (dataByte >> (31 - bitIndex)) & 0x01; // Extracteer huidig bit
            burstCounter = (currentBit == 1) ? BURST_1_DURATION : BURST_0_DURATION;
            TCCR0A |= (1 << COM0A0); // Activeer output voor burst
            isPausing = true;        // Start pauze na deze burst
        } else {
            burstCounter = PAUSE_DURATION; // Pauze tussen bits
            isPausing = false;             // Reset pauze status
            TCCR0A &= ~(1 << COM0A0);      // Ontkoppel Timer0 van OC0A
            bitIndex++;                    // Ga naar het volgende bit
        }
    } else {
        sending = false;        // Stop verzending als alle bits zijn verstuurd
        sendingStartBit = true; // Reset voor de volgende transmissie
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

    if (receivedBits == 32) {
        newDataAvailable = true; // Zet de vlag voor nieuwe data
        receivedBits = 0;        // Reset teller voor ontvangen bits
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
    dataByte = 0b10101010101010101010101010101010;
}