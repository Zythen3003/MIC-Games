// /*
//  * read data from nunchuk and write to Serial
//  */
// #include <avr/io.h>
// #include <util/delay.h>
// #include <avr/interrupt.h>
// #include <Wire.h>
// #include <HardwareSerial.h>
// #include <Nunchuk.h>

// #include <avr/io.h>
// #include <avr/interrupt.h>
// #include <Adafruit_ILI9341.h>

// // Define the pins for the ILI9341
// #define TFT_CS     10
// #define TFT_DC     9

// // Initialize ILI9341 with the pins defined above
// #define NUNCHUK_ADDRESS 0x52
// #define WAIT		1000
// #define BAUDRATE	9600
// #define CHUNKSIZE	32
// #define BUFFERLEN	256

// // prototypes
// int getColor();

// Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

// /*
//  * main
//  */
// int main(void) {
// 	// enable global interrupts
// 	sei();

// 	// join I2C bus as master
// 	Wire.begin();

// 	Nunchuk.begin(NUNCHUK_ADDRESS);

// 	tft.begin();
// 	tft.setRotation(1);  // Adjust as needed for display orientation
// 	tft.fillScreen(ILI9341_BLACK);

// 	// endless loop
// 	uint8_t prevX, prevY, curX, curY;
// 	while(1) {
// 		if (!Nunchuk.getState(NUNCHUK_ADDRESS)) {
// 			return(1);
// 		}

// 		curX = Nunchuk.state.joy_x_axis;
// 		curY = -map(Nunchuk.state.joy_y_axis, 0, 255, 0, 240) + 240;

// 		if (prevX != curX || prevY != curY)
// 			tft.fillCircle(map(prevX, 0, 255, 0, 320), prevY, 10, ILI9341_BLACK);

// 		tft.fillCircle(map(curX, 0, 255, 0, 320), curY, 10, getColor());

// 		prevX = curX;
// 		prevY = curY;
// 	}

// 	return(0);
// }

// int getColor() {
// 	if (Nunchuk.state.c_button)
// 		return ILI9341_MAGENTA;

// 	if (Nunchuk.state.z_button)
// 		return ILI9341_DARKGREEN;

// 	return ILI9341_BLUE;
// }
