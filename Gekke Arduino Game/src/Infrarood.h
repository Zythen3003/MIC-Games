#ifndef INFRAROOD_H
#define INFRAROOD_H

#include <stdint.h>
#include <avr/io.h>

class InfraRood {
public:
    InfraRood();
    void begin(); // Init timers en interrupts
    void send(uint8_t data);
    uint8_t receive();
    bool available(); // Controleer op ontvangen data
    void processISR(); // Wordt aangeroepen vanuit ISR

private:
    void sendPulse(bool state, uint16_t duration);
    void resetReceiveBuffer();
    uint8_t decodeReceivedBits();

    // Buffer voor ontvangen data
    volatile uint8_t receiveBuffer[8];
    volatile uint8_t bufferHead;
    volatile uint8_t bufferTail;

    // Voor zenden
    uint16_t sendTiming[2]; // Pulsduur voor 1 en 0
};

#endif // INFRAROOD_H
