#include "buzzer.h"

// Static member variables
volatile bool Buzzer::isPlaying = false;  // Flag indicating if tone is playing
volatile unsigned long Buzzer::toneEndTicks = 0;  // Time when the tone should stop
volatile unsigned long Buzzer::timerTicks = 0;  // Global tick counter

// Timer1 interrupt handler: toggles the buzzer pin for tone and increments timerTicks
ISR(TIMER1_COMPA_vect) {
  if (Buzzer::isPlaying) {
    PORTD ^= (1 << BUZZER_PIN);  // Toggle buzzer pin to generate tone
  }
  
  Buzzer::timerTicks++;  // Increment global timer tick
}

// Constructor: sets up the buzzer and Timer1
Buzzer::Buzzer() {
  DDRD |= (1 << BUZZER_PIN);  // Set buzzer pin as output
  PORTD &= ~(1 << BUZZER_PIN);  // Ensure buzzer is off
  setupTimer1(1000);  // Initialize Timer1 for timing
}

// Setup Timer1 to generate frequency-based interrupts for tone
void Buzzer::setupTimer1(unsigned int frequency) {
  if (frequency == 0) return;  // Skip if frequency is 0

  TCCR1B = 0;  // Stop Timer1
  TCCR1A = 0;  // Clear Timer1 control registers
  TIMSK1 &= ~(1 << OCIE1A);  // Disable Timer1 interrupt

  // Choose a prescaler for the desired frequency range
  unsigned int prescaler;
  if (frequency > 250) {
      prescaler = 8;           // Higher frequencies, smaller prescaler
      TCCR1B |= (1 << CS11);
  } else {
      prescaler = 64;          // Lower frequencies, larger prescaler
      TCCR1B |= (1 << CS11) | (1 << CS10);
  }

  TCCR1B |= (1 << WGM12);      // CTC mode

  OCR1A = (F_CPU / (2 * prescaler * frequency)) - 1;// Calculate compare value for given frequency

  TIMSK1 |= (1 << OCIE1A);  // Enable Timer1 compare match interrupt

}

// If a tone is already playing, stop it before starting the new one
void Buzzer::playTone(unsigned int frequency, unsigned long duration) {
    if (isPlaying) {
        noTone();  // Stop the current tone
    }
    isPlaying = true;  // Set playing flag to true
    setupTimer1(frequency);  // Setup Timer1 for tone generation
    toneEndTicks = timerTicks + duration;
}

// Update function to check if tone should stop
void Buzzer::update() {
  if (isPlaying && timerTicks >= toneEndTicks) {
    noTone();  // Stop tone if time is up
  }
}

// Stop the tone and reset Timer1
void Buzzer::noTone() {
  isPlaying = false;  // Clear playing flag
  TIMSK1 &= ~(1 << OCIE1A);  // Disable Timer1 interrupt
  TCCR1B = 0;  // Stop Timer1
  PORTD &= ~(1 << BUZZER_PIN);  // Ensure buzzer is off
}

