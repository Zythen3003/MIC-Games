#include "Infrarood.h"
#include <avr/io.h>
#include <avr/interrupt.h>

// Variabele beschikbaar maken voor main
uint8_t tempByte = 0;
bool newDataAvailable = false; // Nieuwe data beschikbaar
uint16_t readCount = 0;

uint16_t pulseBuffer[BUFFER_SIZE1];
uint16_t bufferHead = 0;
uint16_t bufferTail = 0;
uint8_t receivedBits = 0;
bool bufferOverflow = false;
bool newPulseAvailable = false;

uint16_t burstCounter = 0;
bool sending = false;

volatile uint8_t currentBit = 0;
volatile uint8_t dataByte = 0;
volatile uint8_t bitIndex = 0;
volatile bool isPausing = false;

bool sendingStartBit = true; // Vlag om bij te houden of startbit wordt verzonden
bool sendingStopBit = true; // Vlag om bij te houden of stopbit wordt verzonden

void sendNextBit()
{
    if (sendingStartBit)
    {
        if (!isPausing)
        {
            // Stuur het startbit (0)
            burstCounter = BURST_start_DURATION;
            TCCR0A |= (1 << COM0A0); // Activeer output voor burst
            isPausing = true;        // Start pauze na deze burst
        }
        else
        {
            burstCounter = BURST_pausestart_DURATION; // Pauze na het startbit
            isPausing = false;                        // Reset pauze status
            TCCR0A &= ~(1 << COM0A0);                 // Ontkoppel Timer0 van OC0A
            sendingStartBit = false;                  // Startbit is verzonden, ga naar databytes
        }
    }
    else if (bitIndex < 8)
    {
        if (!isPausing)
        {
            // Stuur de bits van de databyte
            currentBit = (dataByte >> (7 - bitIndex)) & 0x01; // Extracteer huidig bit
            burstCounter = (currentBit == 1) ? BURST_1_DURATION : BURST_0_DURATION;
            TCCR0A |= (1 << COM0A0); // Activeer output voor burst
            isPausing = true;        // Start pauze na deze burst
        }
        else
        {
            burstCounter = PAUSE_DURATION; // Pauze tussen bits
            isPausing = false;             // Reset pauze status
            TCCR0A &= ~(1 << COM0A0);      // Ontkoppel Timer0 van OC0A
            bitIndex++;                    // Ga naar het volgende bit
        }
    }
    else if (sendingStopBit)
    {
        // Begin met het verzenden van het stopbit (1)
        if (!isPausing)
        {
            burstCounter = 300; // Burst voor stopbit
            TCCR0A |= (1 << COM0A0);         // Activeer output voor burst
            isPausing = true;                // Start pauze na deze burst
        }
        else
        {
            burstCounter = PAUSE_DURATION; // Pauze na stopbit
            isPausing = false;             // Reset pauze status
            TCCR0A &= ~(1 << COM0A0);      // Ontkoppel Timer0 van OC0A
            sendingStopBit = false;         // Stopbit is verzonden
        }
    }
    else
    {
        // Alles is verzonden, reset de status
        sending = false;        // Stop verzending
        sendingStartBit = true; // Reset voor de volgende transmissie
        sendingStopBit = true;
        bitIndex = 0;
    }
}

// Functie om ontvangen pulsen te verwerken
void processPulse(uint16_t pulseDuration)
{
    // Serial.print("Pulse breedte: ");
    // Serial.println(pulseDuration);

    //  drempelwaarde die duration vergelijkt waarna tempByte een 1 of een 0 krijgt
    if (80 < pulseDuration && pulseDuration < 110)
    {
        // debug pulsbreedte:
        Serial.print("Pulse breedte: ");
        Serial.println(pulseDuration);

        tempByte = (tempByte << 1) | 0;
        receivedBits++;
    }
    else if (130 < pulseDuration && pulseDuration < 160)
    {
        // debug pulsbreedte:
        Serial.print("Pulse breedte: ");
        Serial.println(pulseDuration);

        tempByte = (tempByte << 1) | 1;
        receivedBits++;
    }

    //  als er 8 bits zijn ontvangen kan de data worden uitgelezen in main door boolean newdataAvailable
    if (receivedBits == 8)
    {
        Serial.print("tempByte: ");
        Serial.println(tempByte, BIN);
        newDataAvailable = true; // Zet de vlag voor nieuwe data
        receivedBits = 0;        // Reset teller voor ontvangen bits
    }
}

// Verwerk de ontvangen pulsen
void processReceivedPulses()
{
    if (newPulseAvailable)
    {
        cli(); // Zet interrupts uit voor veilige toegang tot de buffer
        while (bufferTail != bufferHead)
        {
            uint16_t pulseDuration = pulseBuffer[bufferTail];
            bufferTail = (bufferTail + 1) % BUFFER_SIZE1;
            sei(); // Zet interrupts weer aan
            processPulse(pulseDuration);
            cli(); // Zet interrupts uit voor veilige toegang tot de buffer
        }
        newPulseAvailable = false;
        sei(); // Zet interrupts weer aan
    }

    if (bufferOverflow)
    {
        Serial.println("Buffer overflow!");
        bufferOverflow = false;
        receivedBits = 0;
    }
}

// Functie om de databyte te verzenden
void sendDataByte(uint8_t data)
{
    dataByte = data; // Stel de te verzenden data in
    sending = true;
    bitIndex = 0;
    isPausing = false;
    sendingStartBit = true;
    sendNextBit(); // Start de verzending van de eerste bit
}
