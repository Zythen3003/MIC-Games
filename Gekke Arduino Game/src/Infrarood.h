#ifndef INFRAROOD_H
#define INFRAROOD_H

#include <Arduino.h>

// Functies voor initialisatie en verwerking
void initInfrarood();
void processReceivedPulses();


// Externe variabelen
extern uint32_t tempByte;
extern bool newDataAvailable; // Vlag voor nieuwe data

#endif
