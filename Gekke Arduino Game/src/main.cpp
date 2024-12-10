#include <avr/io.h>
#include <avr/interrupt.h>
#include <Arduino.h>

#define SECOND 15624            // Voor 1 seconde interval (bij 16 MHz en prescaler 1024)
#define BURST_1_DURATION 38     // Aantal cycli voor een 1 (1000 µs bij 38 kHz)
#define BURST_0_DURATION 19     // Aantal cycli voor een 0 (500 µs bij 38 kHz)
#define PAUSE_DURATION 19       // Aantal cycli voor 500 µs pauze
#define BUFFER_SIZE 16          // Maximaal aantal pulsen in buffer

volatile uint16_t pulseBuffer[BUFFER_SIZE]; // Buffer voor pulsbreedtes
volatile uint8_t bufferHead = 0;            // Buffer begin
volatile uint8_t bufferTail = 0;            // Buffer einde
volatile bool bufferOverflow = false;       // Indiceert overflow
volatile bool newPulseAvailable = false;    // Indiceert nieuwe puls

volatile uint8_t burstCounter = 0; // Teller voor burstduur
volatile bool sending = false;     // Indiceert of er een byte wordt verzonden
volatile uint8_t currentBit = 0;   // Huidig bit dat wordt verzonden
volatile uint8_t dataByte = 0;     // Byte die moet worden verzonden
volatile uint8_t bitIndex = 0;     // Huidige bitindex (0-7)
volatile bool isPausing = false;   // Pauzeertussen bits

volatile uint8_t receivedBits = 0; // Teller voor aantal ontvangen bits
volatile uint8_t tempByte = 0;     // Tijdelijke buffer voor ontvangen bits
volatile bool newDataAvailable = false; // Indiceert of er nieuwe data beschikbaar is

void timer0Setup(void) {
    TCCR0A = 0;                // Reset Timer0 Control Register A
    TCCR0B = 0;                // Reset Timer0 Control Register B
    TCCR0A |= (1 << WGM01);    // Zet Timer0 in CTC-modus
    OCR0A = 52;                // Stel compare match value in voor 38 kHz
    TIMSK0 |= (1 << OCIE0A);   // Schakel interrupt in voor Timer0 compare match
    TCCR0B |= (1 << CS01);     // Prescaler instellen op 8 voor 38 kHz
}

void timer1Setup(void) {
    TCCR1A = 0;   
    TCCR1B = (1 << CS12) | (1 << CS10) | (1 << WGM12); // CTC-modus, prescaler 1024
    TCCR1C = 0;
    OCR1A = SECOND;            // Vergelijkwaarde voor 1 seconde
    TCNT1 = 0;
    TIMSK1 = (1 << OCIE1A);    // Schakel interrupt in voor Timer1 compare match
}

void setupInterrupt0(void) {
    EICRA |= (1 << ISC01); // Falling edge trigger voor INT0
    EIMSK |= (1 << INT0);  // Schakel INT0 interrupt in
    DDRD &= ~(1 << DDD2);  // Zet PD2 (INT0) als ingang
    PORTD |= (1 << PORTD2); // Activeer interne pull-up weerstand
}

void sendNextBit() {
    if (bitIndex < 8) {
        if (!isPausing) {
            currentBit = (dataByte >> (7 - bitIndex)) & 0x01; // Extracteer huidig bit (MSB eerst)
            if (currentBit == 1) {
                burstCounter = BURST_1_DURATION; // Stel burstduur in voor 1
            } else {
                burstCounter = BURST_0_DURATION; // Stel burstduur in voor 0
            }
            TCCR0A |= (1 << COM0A0); // Verbind Timer0 met pin OC0A (PD6)
            isPausing = true;        // Start pauzetoestand na deze burst
        } else {
            burstCounter = PAUSE_DURATION; // Stel pauzeduur in
            isPausing = false;            // Ga na de pauze naar het volgende bit
            TCCR0A &= ~(1 << COM0A0);     // Ontkoppel Timer0 van pin OC0A tijdens pauze
            bitIndex++;                   // Ga naar het volgende bit
        }
    } else {
        sending = false; // Stop met verzenden als alle bits zijn verstuurd
    }
}

ISR(TIMER1_COMPA_vect) {
    if (!sending) {
        sending = true;
        bitIndex = 0;          // Reset de bitindex
        isPausing = false;     // Begin zonder pauze
        sendNextBit();         // Start met het eerste bit
    }
}

ISR(TIMER0_COMPA_vect) {
    if (burstCounter > 0) {
        burstCounter--;         // Verlaag de burstteller
        if (burstCounter == 0) {
            if (sending) {
                sendNextBit();  // Stuur het volgende bit of pauze
            }
        }
    }
}

ISR(INT0_vect) {
  //led lampje gaat branden op poort 13 bij een inkomende interupt
  PORTB ^= (1 << PORTB5);
  
    static uint16_t lastFallingEdgeTime = 0;
    uint16_t currentTime = TCNT1;

    // Bereken pulsduur
    uint16_t pulseDuration = currentTime - lastFallingEdgeTime;
    lastFallingEdgeTime = currentTime;

    // Voeg pulsduur toe aan buffer
    uint8_t nextHead = (bufferHead + 1) % BUFFER_SIZE;
    if (nextHead == bufferTail) {
        bufferOverflow = true; // Buffer overflow detecteren
    } else {
        pulseBuffer[bufferHead] = pulseDuration;
        bufferHead = nextHead;
        newPulseAvailable = true;
    }
}

void processPulse(uint16_t pulseDuration) {
    static uint8_t tempByte = 0;      // Tijdelijke buffer voor bits
    static uint8_t receivedBits = 0; // Aantal ontvangen bits

    // Debug: Log pulsbreedte
    Serial.print("Pulse breedte: ");
    Serial.println(pulseDuration);

    // Stel drempelwaarden in voor 0 en 1
    if (pulseDuration <= 19) { // Drempel voor logische 0
        tempByte = (tempByte << 1) | 0;
        receivedBits++;
    } else if (pulseDuration >= 20) { // Drempel voor logische 1
        tempByte = (tempByte << 1) | 1;
        receivedBits++;
    }

    // Controleer of een volledige byte is ontvangen
    if (receivedBits == 8) {
        Serial.print("Ontvangen byte: ");
        Serial.println(tempByte, BIN);
        tempByte = 0;       // Reset buffer
        receivedBits = 0;   // Reset teller
    }
}


void setup(void) {
    DDRD |= (1 << DDD6);       // Zet PD6 (OC0A) als uitgang
    DDRD &= ~(1 << DDD2);      // Zet PD2 (INT0) als ingang
    DDRB |= (1 << DDB5);       // Zet pin 13 (LED) als uitgang
    timer0Setup();             // Initialiseer Timer0
    timer1Setup();             // Initialiseer Timer1
    setupInterrupt0();         // Initialiseer interrupt op pin 2
    Serial.begin(9600);        // Start seriële communicatie
    sei();                     // Globale interrupts aan
    dataByte = 0b10101010;     // Testbyte: patroon van 1's en 0's
}

int main(void) {
    setup();
    while (1) {
        if (newPulseAvailable) {
            cli(); // Schakel interrupts tijdelijk uit
            while (bufferTail != bufferHead) {
                uint16_t pulseDuration = pulseBuffer[bufferTail];
                bufferTail = (bufferTail + 1) % BUFFER_SIZE;
                sei(); // Sta interrupts toe tijdens verwerking
                processPulse(pulseDuration); // Verwerk de pulsduur
                cli(); // Schakel interrupts weer uit
            }
            newPulseAvailable = false; // Reset vlag
            sei(); // Schakel interrupts weer in
        }

        if (bufferOverflow) {
            Serial.println("Buffer overflow!");
            bufferOverflow = false; // Reset overflow melding
        }
    }
    return 0;
}