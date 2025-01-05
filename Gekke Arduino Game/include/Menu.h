#ifndef MENU_H
#define MENU_H

#include <Adafruit_ILI9341.h>
#include <Adafruit_FT6206.h>

// Menu class definition
class Menu {
public:
    Menu(Adafruit_ILI9341* tft);

    void handleMenuInput(int ticksSinceLastUpdate, int currentLevel); // Handles menu input
    void handleTouchInput(TS_Point tPoint, int ticksSinceLastUpdate, int currentLevel); // Handles touch input
    void displayEndGameMessage(int currentLevel); // Display the end game message
    void displayLevelMessage(int currentLevel); // Display the end game message
    void drawMenu(); // Draws the entire menu
    void updateSelection(int direction); // Updates the selection (-1 for up, 1 for down)
    int getSelectedOption(); // Returns the currently selected option
    void drawOption(int optionIndex, const char* text, bool selected); // Draw specific option
    int saveHighScore(int address, int newTime);
    int readIntFromEEPROM(int address);
    void loadHighScores();
    bool isSinglePlayer; // True for singleplayer, false for multiplayer
    
private:
    Adafruit_ILI9341* tft;
    int selectedOption; // Current selected option (0 = Singleplayer, 1 = Multiplayer)
    bool isNewHighScore = false;
    int savedHighScoreSingle = 0;
    int totalTime = 0;
    int savedHighScoreMulti = 0;
    int singlePlayerHighScoreAddress = 10;
    int multiPlayerHighScoreAddress = 15;
    int previousSelectedOption;  // The previous selected option (for redrawing)
    
    void startSingleplayer(int ticksSinceLastUpdate, int currentLevel);
    void startMultiplayer();
};

extern bool gameStarted;

#endif // MENU_H
