#ifndef BUZZER_H  
#define BUZZER_H

#include <Arduino.h> 

#define BUZZER_PIN 3  // Define the pin number connected to the buzzer (pin 3)


class Buzzer {
public:
    // Static member variables to keep track of the state
    static volatile unsigned long timerTicks;  // Global tick counter (incremented every 1ms)
    static volatile bool isPlaying;  // Flag indicating if the buzzer is currently playing a tone
    static volatile unsigned long toneEndTicks;  // Time when the tone should stop (in timer ticks)
    typedef unsigned int Frequency;// Type alias for Frequency (using unsigned int for frequency values)

    Buzzer();// Constructor: Initializes the buzzer and related timers
    void update();    // Update method to track time and manage tone playback
    static void noTone(); // Static method to stop the tone (turn off the buzzer)
    void playTone(Frequency frequency, unsigned long duration);   // Method to play a tone with a specified frequency and duration
    void nonBlockingDelay(unsigned long duration); // Method to delay execution without blocking

private:
    void setupTimer1(Frequency frequency);// Private method to setup Timer1 for generating tone frequency
};

#endif
