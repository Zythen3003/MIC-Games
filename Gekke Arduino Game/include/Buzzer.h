#ifndef BUZZER_H  
#define BUZZER_H

#include <avr/io.h>
#include <avr/interrupt.h>

// Define the buzzer pin
#define BUZZER_PIN 3 

class Buzzer {
public:
    Buzzer();  // Constructor to initialize the buzzer
    void playTone(unsigned int frequency, unsigned long duration);// Function to play a tone with specified frequency (Hz) and duration (ms)
    void noTone();  // Function to stop any ongoing tone
    void update();  // Function to update the tone state

    // Static member variables (shared among all instances)
    static volatile bool isPlaying;          // Flag to indicate if a tone is playing
    static volatile unsigned long toneEndTicks;  // Timer tick count when tone should stop
    static volatile unsigned long timerTicks;    // Current timer tick count

private:
    void setupTimer1(unsigned int frequency);  // Function to configure Timer1 for a specific frequency
};

#endif  // BUZZER_H
