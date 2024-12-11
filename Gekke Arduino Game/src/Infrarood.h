#ifndef INFRAROOD_H
#define INFRAROOD_H

#include <Arduino.h>

// Functies voor initialisatie en verwerking
void initInfrarood();
void processReceivedPulses();
void sendDataByte(uint32_t data);  // Voeg deze functie toe om de data te verzenden

// Externe variabelen
extern uint32_t tempByte;
extern bool newDataAvailable; // Vlag voor nieuwe data
extern uint16_t readCount;

#endif
