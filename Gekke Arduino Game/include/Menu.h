#ifndef MENU_H
#define MENU_H

#include <Adafruit_ILI9341.h>

// Menu class definition
class Menu {
public:
    Menu(Adafruit_ILI9341* tft);

    void drawMenu(); // Draws the entire menu
    void updateSelection(int direction); // Updates the selection (-1 for up, 1 for down)
    int getSelectedOption(); // Returns the currently selected option
    void drawOption(int optionIndex, const char* text, bool selected); // Draw specific option
private:
    Adafruit_ILI9341* tft;
    int selectedOption; // Current selected option (0 = Singleplayer, 1 = Multiplayer)
    int previousSelectedOption;  // The previous selected option (for redrawing)
};

#endif // MENU_H
