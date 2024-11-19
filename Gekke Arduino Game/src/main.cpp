#include <avr/io.h>
#include <avr/interrupt.h>
#include <Adafruit_ILI9341.h>

// Define the pins for the ILI9341
#define TFT_CS     10
#define TFT_DC     9

// Initialize ILI9341 with the pins defined above
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

void setup() {
  tft.begin();
  tft.setRotation(1);  // Adjust as needed for display orientation
  tft.fillScreen(ILI9341_BLACK);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(2);
  tft.setCursor(10, 10);
  tft.println("Hello, ILI9341!");
}

void loop() {
  // Add your drawing code here
}