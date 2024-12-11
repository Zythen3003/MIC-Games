#include "Infrarood.h"
#include <avr/io.h>
#include <avr/interrupt.h>

// Variabele beschikbaar maken voor main
uint32_t tempByte = 0;
bool newDataAvailable = false; // Nieuwe data beschikbaar
uint16_t readCount = 0;

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
    TCCR0B |= (1 << CS01);  // Zet prescaler op 8 voor Timer0
    Serial.println("interuptcheck");
}

void setupInterrupt0(void) {
    EICRA |= (1 << ISC01) | (1 << ISC00); 
    EIMSK |= (1 << INT0); // Zet de interrupt enable voor INT0
    DDRD &= ~(1 << DDD2); // Zet PD2 (INT0) als ingang
    PORTD |= (1 << PORTD2); // Zet pull-up weerstand aan
    Serial.println("interuptcheck");
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

// Interrupt voor Timer0
ISR(TIMER0_COMPA_vect) {
     readCount++;
    if (burstCounter > 0) {
        burstCounter--;
        if (burstCounter == 0 && sending) {
            sendNextBit();
        }
    }
}

// Interrupt voor INT0 (externe interrupt, bijvoorbeeld voor ontvangen signalen)
ISR(INT0_vect) {
    PORTB ^= (1 << PORTB5); // LED knipperen bij ontvangst
    static uint16_t lastFallingEdgeTime = 0;
    uint16_t pulseDuration = readCount - lastFallingEdgeTime;
    lastFallingEdgeTime = readCount;

    uint8_t nextHead = (bufferHead + 1) % BUFFER_SIZE;
    if (nextHead == bufferTail) {
        bufferOverflow = true;
    } else {
        pulseBuffer[bufferHead] = pulseDuration;
        bufferHead = nextHead;
        newPulseAvailable = true;
    }
}

// Functie om ontvangen pulsen te verwerken
void processPulse(uint16_t pulseDuration) {
    static uint8_t receivedBits = 0;

    Serial.print("Pulse breedte: ");
    Serial.println(pulseDuration);

    if (pulseDuration <= 50) {
        tempByte = (tempByte << 1) | 0;
        receivedBits++;
    } else if (pulseDuration >= 51) {
        tempByte = (tempByte << 1) | 1;
        receivedBits++;
    }

    if (receivedBits == 32) {
        newDataAvailable = true; // Zet de vlag voor nieuwe data
        receivedBits = 0;        // Reset teller voor ontvangen bits
    }
}

// Verwerk de ontvangen pulsen
void processReceivedPulses() {
    if (newPulseAvailable) {
        cli();  // Zet interrupts uit voor veilige toegang tot de buffer
        while (bufferTail != bufferHead) {
            uint16_t pulseDuration = pulseBuffer[bufferTail];
            bufferTail = (bufferTail + 1) % BUFFER_SIZE;
            sei(); // Zet interrupts weer aan
            processPulse(pulseDuration);
            cli();  // Zet interrupts uit voor veilige toegang tot de buffer
        }
        newPulseAvailable = false;
        sei(); // Zet interrupts weer aan
    }

    if (bufferOverflow) {
        Serial.println("Buffer overflow!");
        bufferOverflow = false;
    }
}

// Initialisatie van de IR-module
void initInfrarood() {
    DDRD |= (1 << DDD6);  // Zet PD6 (OC0A) als uitgang voor IR
    DDRD &= ~(1 << DDD2); // Zet PD2 (INT0) als ingang voor ontvangen signalen
    DDRB |= (1 << DDB5);  // Zet pin 13 (LED) als uitgang

    timer0Setup();  // Zet Timer0 op voor burst
    setupInterrupt0(); // Zet externe interrupt voor ontvangst
}

// Functie om de databyte te verzenden
void sendDataByte(uint32_t data) {
    dataByte = data;  // Stel de te verzenden data in
    sending = true;
    bitIndex = 0;
    isPausing = false;
    sendNextBit(); // Start de verzending van de eerste bit
}
