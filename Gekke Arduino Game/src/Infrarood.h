#ifndef INFRAROOD_H
#define INFRAROOD_H

#include <Arduino.h>

#define BUFFER_SIZE1 8

// Functies voor initialisatie en verwerking
void initInfrarood();
void processReceivedPulses();
void sendDataByte(uint8_t data); 
void sendNextBit(); // Voeg deze functie toe om de data te verzenden

// Externe variabelen
extern uint8_t tempByte;
extern bool newDataAvailable; // Vlag voor nieuwe data
extern uint8_t readCount;
extern uint16_t burstCounter;
extern bool sending;
extern uint8_t bufferHead;
extern uint8_t bufferTail;
extern bool bufferOverflow;
extern uint8_t pulseBuffer[BUFFER_SIZE1];
extern bool newPulseAvailable;
#endif
