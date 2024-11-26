#include <avr/io.h>
#include <avr/interrupt.h>
#include <Adafruit_ILI9341.h>

// Define the pins for the ILI9341
#define TFT_CS     10 // Chip Select pin for SPI communication
#define TFT_DC     9  // Data/Command pin to distinguish data from instructions
#define TFT_RST    8  // Reset pin for hardware reset (optional, can be omitted)

// Initialize ILI9341 with the pins defined above
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  // Initialize the display and start communication
  tft.begin();

  // Set the rotation of the display (0-3), determines the orientation of graphics/text
  tft.setRotation(1);

  // Clear the display with a black screen
  tft.fillScreen(ILI9341_BLACK);

  // Set the text color to white for good visibility
  tft.setTextColor(ILI9341_WHITE);

  // Set the size of the text (1 = smallest, higher numbers = larger text)
  tft.setTextSize(2);

  // Set the cursor position for the text to be drawn (x = 10, y = 10)
  tft.setCursor(10, 10);

  // Print a message to the screen starting at the cursor position
  tft.println("Hello, ILI9341");
}

void loop() {
  // Nothing implemented here, as the display is static in this example
}