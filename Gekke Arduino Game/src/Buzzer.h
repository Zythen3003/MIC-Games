#ifndef BUZZER_H
#define BUZZER_H

#include <Arduino.h>

#define BUZZER_PIN 3
#define TIMER_PRESCALER 8      // Prescaler voor Timer2

using Frequency = unsigned int;

class Buzzer {
private:
    void setupTimer2(Frequency frequency);

public:
    Buzzer();
    void playMarioCoinSound();
    static volatile bool isPlaying; // Static member for isPlaying
    void playTone(Frequency frequency, unsigned long duration);
    void noTone();
    static volatile unsigned long toneEndTime; // Correct placement: inside the class, but not in private or public

};

#endif