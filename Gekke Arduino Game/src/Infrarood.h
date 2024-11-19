#ifndef Infrarood_H
#define Infrarood_H

#include "Arduino.h"
#include "SoftwareSerial.h"

class Infrarood {
public:
	Infrarood();

	SoftwareSerial _serial;

	void send(char message);
	char receive();

private:
	char prevChar;
};

#endif