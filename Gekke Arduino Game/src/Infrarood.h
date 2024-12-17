#ifndef INFRAROOD_H
#define INFRAROOD_H

#include <Arduino.h>

// Functies voor initialisatie en verwerking
void initInfrarood();
void processReceivedPulses();
void sendDataByte(uint8_t data);  // Voeg deze functie toe om de data te verzenden

// Externe variabelen
extern uint8_t tempByte;
extern bool newDataAvailable; // Vlag voor nieuwe data
extern uint8_t readCount;

#endif
