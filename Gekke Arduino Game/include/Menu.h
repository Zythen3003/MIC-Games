#ifndef MENU_H
#define MENU_H

#include <Adafruit_ILI9341.h>
#include <Adafruit_FT6206.h>

// Menu class definition
class Menu {
public:
    Menu(Adafruit_ILI9341* tft);

    void handleMenuInput(); // Handles menu input
    void handleTouchInput(TS_Point tPoint);
    void displayEndGameMessage(); // Display the end game message
    void drawMenu(); // Draws the entire menu
    void startMultiplayerGame();

    bool isSinglePlayer; // True for singleplayer, false for multiplayer
    
private:
    Adafruit_ILI9341* tft;
    int selectedOption; // Current selected option (0 = Singleplayer, 1 = Multiplayer)
    bool isNewHighScore = false;
    int savedHighScoreSingle = 0;
    int savedHighScoreMulti = 0;
    int singlePlayerHighScoreAddress = 10;
    int multiPlayerHighScoreAddress = 15;
    int previousSelectedOption;  // The previous selected option (for redrawing)
    void drawOption(int optionIndex, const char* text, bool selected); // Draw specific option
    void loadHighScores();
    int readIntFromEEPROM(int address);
    int saveHighScore(int address, int newTime);
    void updateSelection(int direction); // Updates the selection (-1 for up, 1 for down)

    void startSingleplayer();
    void startMultiplayer();
};

extern bool gameStarted;

#endif // MENU_H
