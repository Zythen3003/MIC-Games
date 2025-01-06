#ifndef INFRAROOD_H
#define INFRAROOD_H

#include <Arduino.h>

#define BUFFER_SIZE1 16
#define BURST_1_DURATION 38 
#define BURST_0_DURATION 19
#define BURST_start_DURATION 342
#define BURST_pausestart_DURATION 171
#define PAUSE_DURATION 19

// Functies voor initialisatie en verwerking
void initInfrarood();
void processReceivedPulses();
void sendDataByte(uint8_t data); 
void sendNextBit(); // Voeg deze functie toe om de data te verzenden

// Externe variabelen
extern uint8_t tempByte;
extern bool newDataAvailable; // Vlag voor nieuwe data
extern uint16_t readCount;
extern uint16_t burstCounter;
extern bool sending;
extern uint16_t bufferHead;
extern uint16_t bufferTail;
extern bool bufferOverflow;
extern uint16_t pulseBuffer[BUFFER_SIZE1];
extern bool newPulseAvailable;
#endif
