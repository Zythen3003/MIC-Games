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
void Buzzer::setupTimer1(Frequency frequency) {
  if (frequency == 0) return;  // Skip if frequency is 0

  TCCR1B = 0;  // Stop Timer1
  TCCR1A = 0;  // Clear Timer1 control registers
  TIMSK1 &= ~(1 << OCIE1A);  // Disable Timer1 interrupt

  TCCR1B |= (1 << CS21);  // Set Timer1 prescaler to 8
  TCCR1A |= (1 << WGM21);  // Set Timer1 to CTC mode

  OCR1A = (F_CPU / (8 * 2 * frequency)) - 1;  // Calculate compare value for given frequency

  TIMSK1 |= (1 << OCIE1A);  // Enable Timer1 compare match interrupt

    // Debugging: print the OCR1A value and frequency
  //Serial.print("Timer1 set up with frequency: ");
  //Serial.print(frequency);
  //Serial.print(" Hz, OCR1A: ");
  //Serial.println(OCR1A);
}

// Play a tone with specified frequency and duration
void Buzzer::playTone(Frequency frequency, unsigned long duration) {
  if (frequency == 0) return;  // Skip if frequency is 0

  duration = duration * 1.5;  // Increase duration by 50%

  // If a tone is already playing, stop it before starting the new one
  if (isPlaying) {
    noTone();  // Stop the current tone
  }

  isPlaying = true;  // Set playing flag to true
  setupTimer1(frequency);  // Setup Timer1 for tone generation
  toneEndTicks = timerTicks + duration;  // Calculate and store when tone should end

    // Debugging: print when the tone starts and its duration
  //Serial.print("Playing tone at ");
  //Serial.print(frequency);
  //Serial.print(" Hz for ");
  //Serial.print(duration);
  //Serial.println(" ms.");
}

// Update function to check if tone should stop
void Buzzer::update() {
  if (isPlaying && timerTicks >= toneEndTicks) {
    noTone();  // Stop tone if time is up
  }
   // Debugging: print the current timerTicks and toneEndTicks
  if (isPlaying) {
    //Serial.print("timerTicks: ");
    //Serial.print(timerTicks);
    //Serial.print(", toneEndTicks: ");
    //Serial.println(toneEndTicks);
  }
}

// Stop the tone and reset Timer1
void Buzzer::noTone() {
  isPlaying = false;  // Clear playing flag
  TIMSK1 &= ~(1 << OCIE1A);  // Disable Timer1 interrupt
  TCCR1B = 0;  // Stop Timer1
  PORTD &= ~(1 << BUZZER_PIN);  // Ensure buzzer is off
}


// Function to create a non-blocking delay
void Buzzer::nonBlockingDelay(unsigned long duration) {
    unsigned long startTime = Buzzer::timerTicks;
    while (Buzzer::timerTicks - startTime < duration);
}