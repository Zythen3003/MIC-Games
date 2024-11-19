/*
 * read data from nunchuk and write to Serial
 */
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <Wire.h>
#include <HardwareSerial.h>
#include <Nunchuk.h>

#include <avr/io.h>
#include <avr/interrupt.h>
#include <Adafruit_ILI9341.h>

// Define the pins for the ILI9341
#define TFT_CS     10
#define TFT_DC     9

// Initialize ILI9341 with the pins defined above
#define NUNCHUK_ADDRESS 0x52
#define WAIT		1000
#define BAUDRATE	9600
#define CHUNKSIZE	32
#define BUFFERLEN	256

// what to show
#define STATE
//#define MEM
//#define CAL

// prototypes
bool show_memory(void);
bool show_state(void);
bool show_calibration(void);
int getColor();

Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

void loop() {
  // Add your drawing code here
}

/*
 * main
 */
int main(void) {
	// enable global interrupts
	sei();

	// use Serial for printing nunchuk data
	Serial.begin(BAUDRATE);

	// join I2C bus as master
	Wire.begin();

	// handshake with nunchuk
	Serial.print("-------- Connecting to nunchuk at address 0x");
	Serial.println(NUNCHUK_ADDRESS, HEX);
	if (!Nunchuk.begin(NUNCHUK_ADDRESS))
	{
		Serial.println("******** No nunchuk found");
		Serial.flush();
		return(1);
	}

	/*
	 * get identificaton (nunchuk should be 0xA4200000)
	 */
	Serial.print("-------- Nunchuk with Id: ");
	Serial.println(Nunchuk.id);

	tft.begin();
	tft.setRotation(1);  // Adjust as needed for display orientation
	tft.fillScreen(ILI9341_BLACK);
	tft.setTextColor(ILI9341_WHITE);
	tft.setTextSize(2);
	tft.setCursor(10, 10);
	tft.println("Hello, ILI9341!");
	tft.setCursor(0, 0);

	// tft.drawRect(0, 0, 320, 240, ILI9341_CYAN);

	// endless loop
	uint8_t prevX;
	uint8_t prevY;
	while(1) {
		if (!show_state()) {
			Serial.println("******** No nunchuk found");
			Serial.flush();
			return(1);
		}

		tft.fillCircle(map(prevX, 0, 255, 0, 320), prevY, 10, ILI9341_BLACK);
		prevX = Nunchuk.state.joy_x_axis;
		prevY = -map(Nunchuk.state.joy_y_axis, 0, 255, 0, 240) + 240;
		tft.fillCircle(map(prevX, 0, 255, 0, 320), prevY, 10, getColor());

		// wait a while
		// _delay_ms(WAIT);
	}

	return(0);
}

int getColor() {
	if (Nunchuk.state.c_button)
		return ILI9341_MAGENTA;

	if (Nunchuk.state.z_button)
		return ILI9341_DARKGREEN;

	return ILI9341_BLUE;
}

bool show_memory(void)
{
	// print whole memory
	Serial.println("------ Whole memory------------------------");
	for (uint16_t n = 0; n < BUFFERLEN; n += CHUNKSIZE) {
		// read
		if (Nunchuk.read(NUNCHUK_ADDRESS, (uint8_t)n,
				(uint8_t)CHUNKSIZE) != CHUNKSIZE)
			return (false);

		// print
		Serial.print("0x");
		if (n == 0) Serial.print("0");
		Serial.print(n, HEX);
		Serial.print(": ");
		for (uint8_t i = 0; i < CHUNKSIZE; i++) {
			if (Nunchuk.buffer[i] == 0)
				Serial.print('0');
			Serial.print(Nunchuk.buffer[i], HEX);
		}
		Serial.println("");
	}

	return(true);
}

bool show_state(void)
{
	if (!Nunchuk.getState(NUNCHUK_ADDRESS))
		return (false);
// 	Serial.println("------State data--------------------------");
// 	Serial.print("Joy X: ");
// 	Serial.print(Nunchuk.state.joy_x_axis, HEX);
// 	Serial.print("\t\tAccel X: ");
// 	Serial.print(Nunchuk.state.accel_x_axis, HEX);
// 	Serial.print("\t\tButton C: ");
// 	Serial.println(Nunchuk.state.c_button, HEX);

// 	Serial.print("Joy Y: ");
// 	Serial.print(Nunchuk.state.joy_y_axis, HEX);
// 	Serial.print("\t\tAccel Y: ");
// 	Serial.print(Nunchuk.state.accel_y_axis, HEX);
// 	Serial.print("\t\tButton Z: ");
// 	Serial.println(Nunchuk.state.z_button, HEX);

// 	Serial.print("\t\t\tAccel Z: ");
// 	Serial.println(Nunchuk.state.accel_z_axis, HEX);

//   Serial.print("Button C: ");
//   Serial.println(Nunchuk.state.c_button ? "Pressed" : "Released");

//   Serial.print("Button Z: ");
//   Serial.println(Nunchuk.state.z_button ? "Pressed" : "Released");

	return(true);
}

bool show_calibration(void)
{
	if (!Nunchuk.getCalibration(NUNCHUK_ADDRESS))
		return(false);
	Serial.println("------Calibration data (unused)-----------");
	Serial.print("X-0G: 0x");
	Serial.print(Nunchuk.cal.x0, HEX);
	Serial.print("\tY-0G: 0x");
	Serial.print(Nunchuk.cal.y0, HEX);
	Serial.print("\tZ-0G: 0x");
	Serial.println(Nunchuk.cal.z0, HEX);
	Serial.print("X-1G: 0x");
	Serial.print(Nunchuk.cal.x1, HEX);
	Serial.print("\tY-1G: 0x");
	Serial.print(Nunchuk.cal.y1, HEX);
	Serial.print("\tZ-1G: 0x");
	Serial.println(Nunchuk.cal.z1, HEX);
	Serial.print("xmin: 0x");
	Serial.print(Nunchuk.cal.xmin, HEX);
	Serial.print("\txmax: 0x");
	Serial.print(Nunchuk.cal.xmax, HEX);
	Serial.print("\txcenter: 0x");
	Serial.println(Nunchuk.cal.xcenter, HEX);
	Serial.print("ymin: 0x");
	Serial.print(Nunchuk.cal.ymin, HEX);
	Serial.print("\tymax: 0x");
	Serial.print(Nunchuk.cal.ymax, HEX);
	Serial.print("\tycenter: 0x");
	Serial.println(Nunchuk.cal.ycenter, HEX);
	Serial.print("chksum: 0x");
	Serial.println(Nunchuk.cal.chksum, HEX);

	return(true);
}
