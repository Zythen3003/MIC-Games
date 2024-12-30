#include "menu.h"
#include "Nunchuk.h"
#include "GameMechanics.h"
#include <EEPROM.h>

void Menu::handleMenuInput() {
    if (Nunchuk.getState(NUNCHUK_ADDRESS)) {
        int joyX = Nunchuk.state.joy_x_axis;

        // Navigate menu using joystick input
        if (joyX < 100) { // Left
            updateSelection(-1);
        } else if (joyX > 150) { // Right
            updateSelection(1);
        }

        // Confirm menu selection with the Z button
        if (Nunchuk.state.z_button) {
            int selectedOption = getSelectedOption();

            if (selectedOption == 0) { // Singleplayer selected
                Serial.println("Singleplayer selected");
                isSinglePlayer = true; // Set to singleplayer mode
                gameStarted = true; // Mark game as started
                SetupGrid(); // Call SetupGrid() for the game

                // Add game logic for singleplayer mode here
                startSingleplayer();
            } else if (selectedOption == 1) { // Multiplayer selected
                Serial.println("Multiplayer selected");
                isSinglePlayer = false; // Set to multiplayer mode
                gameStarted = false; // Mark game as started
                tft->fillScreen(ILI9341_DARKGREEN);
                tft->setTextColor(ILI9341_BLACK);
                tft->setTextSize(3);
                tft->setCursor(30, 20);
                tft->println("Multiplayer mode");
                startMultiplayer();
                // Add multiplayer setup code here
            }
        }
    }
}

void Menu::startSingleplayer() {
    gameStarted = true; // Mark game as started
    SetupGrid(); // Call SetupGrid() for the game
    // Add game logic for singleplayer mode here
}

void Menu::startMultiplayer() {
    gameStarted = false; // Mark game as started
    tft->fillScreen(ILI9341_DARKGREEN);
    tft->setTextColor(ILI9341_BLACK);
    tft->setTextSize(3);
    tft->setCursor(30, 20);
    tft->println("Multiplayer mode");
}

bool isTouchInRect(int touchX, int touchY, int rectX, int rectY, int rectWidth, int rectHeight) {
  return (touchX >= rectX && touchX <= (rectX + rectWidth) &&
          touchY >= rectY && touchY <= (rectY + rectHeight));
}

void translateTouchToDisplay(int &touchX, int &touchY) {
    int tempY = touchY;
    touchY = touchX;   // Touchscreen Y to display X
    touchX = map(tempY, 0, SCREEN_WIDTH, SCREEN_WIDTH, 0);
}

void Menu::handleTouchInput(TS_Point tPoint) {
    int touchX = tPoint.x;  // Map Y (raw) to X (display)
    int touchY = tPoint.y; // Map X (raw) to Y (display)

    translateTouchToDisplay(touchX, touchY);
    
    if (isTouchInRect(touchX, touchY, 20, 100, 130, 50)) {
        startSingleplayer();
    }

    if (isTouchInRect(touchX, touchY, 160, 100, 130, 50)) {
        startMultiplayer();
    }
}

// Constructor
Menu::Menu(Adafruit_ILI9341* tft) {
    this->tft = tft;
    this->selectedOption = 0; // Default to Singleplayer
    this->isSinglePlayer = true; // Default game mode is singleplayer
}

// Draws the entire menu
void Menu::drawMenu() {
    // Load the highscores from EEPROM
    loadHighScores();

    tft->fillScreen(ILI9341_DARKGREEN);

    // Draw title
    tft->setTextColor(ILI9341_BLACK);
    tft->setTextSize(3);
    tft->setCursor(30, 20);
    tft->println("TREASURE HUNT");

    // Draw the "Choose your mode" header 
    tft->setTextColor(ILI9341_BLACK);
    tft->setTextSize(2);
    tft->setCursor(30, 80);
    tft->print("Choose your mode");

    // Draw menu options
    drawOption(0, "Solo", selectedOption == 0);
    drawOption(1, "Co-Op", selectedOption == 1);
}

// Updates the selection based on input direction
void Menu::updateSelection(int direction) {
    // Controleer of de selectie moet worden gewijzigd
    if ((selectedOption == 0 && direction < 0) || (selectedOption == 1 && direction > 0)) {
        // Als je al op de rand bent (links of rechts), verander niets
        return;
    }

    // Clear de vorige selectie
    drawOption(selectedOption, selectedOption == 0 ? "Solo" : "Co-Op", false);

    // Werk de selectie bij
    selectedOption += direction;

    // Teken de nieuwe selectie
    drawOption(selectedOption, selectedOption == 0 ? "Solo" : "Co-Op", true);
}
// Returns the currently selected option
int Menu::getSelectedOption() {
    return selectedOption;
}

// Draws a specific menu option
void Menu::drawOption(int optionIndex, const char* text, bool selected) {
    int xPos = 20 + (optionIndex * 140);
    // Save the current time
    if (selected) {
        tft->fillRoundRect(xPos, 100, 130, 50, 10, ILI9341_GREEN);
    } else {
        tft->fillRoundRect(xPos, 100, 130, 50, 10, ILI9341_LIGHTGREY);
    }

    // Draw the option text
    tft->setTextColor(ILI9341_BLACK);
    tft->setTextSize(2);
    tft->setCursor(xPos + 35, 115);
    tft->print(text);

    // Draw the highscore box
    tft->fillRoundRect(xPos, 180, 130, 50, 10, ILI9341_BLACK);
    tft->setTextColor(ILI9341_WHITE);
    tft->setTextSize(2);
    tft->setCursor(xPos + 35, 195);
    if(optionIndex == 0) {
        tft->print(savedHighScoreSingle);
    } else if (optionIndex == 1) {
        tft->print(savedHighScoreMulti);
    }
    // Draw the "Highscore" header 
    tft->setTextColor(ILI9341_WHITE);
    tft->setTextSize(2);
    tft->setCursor(xPos + 20, 160);
    tft->print("Highscore");

}


void Menu::displayEndGameMessage() {
    tft->fillScreen(ILI9341_DARKGREEN); // Clear the screen

    tft->setCursor(SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 2 - 40);
    tft->setTextSize(2);
    tft->setTextColor(ILI9341_BLACK);

    // Display the end game message
    if (player1Score > player2Score) {
        tft->print("Player 1 Wins!");
    } else if (player1Score < player2Score) {
        tft->print("Player 2 Wins!");
    } else {
        tft->print("It's a Draw!");
    }

    saveHighScore(10, gameTime); // Save the high score to EEPROM
    
    // Display the final scores
    tft->setCursor(SCREEN_WIDTH / 2 - 60, SCREEN_HEIGHT / 2);
    tft->print("Score: ");
    tft->print(gameTime);
    tft->print("s");

    // Display a special message if a new high score was achieved
    if (isNewHighScore) {
        tft->setCursor(SCREEN_WIDTH / 2 - 80, SCREEN_HEIGHT / 2 + 20);
        tft->setTextColor(ILI9341_RED);
        tft->print("New High Score!");
    }

    // Display the message to return to the main menu
    tft->setCursor(10, SCREEN_HEIGHT / 2 + 60);
    tft->setTextColor(ILI9341_BLACK);
    tft->setTextSize(1);
    tft->print("Press button to return to Main Menu...");

     // Reset the game state to prepare for the next game or menu
    player1Score = 0;
    player2Score = 0;

}

int Menu::readIntFromEEPROM(int address) {
    byte byte1 = EEPROM.read(address);
    byte byte2 = EEPROM.read(address + 1);
    int value = (byte1 << 8) + byte2;

    // Check for uninitialized EEPROM (0xFFFF)
    if (static_cast<unsigned int>(value) == 0xFFFF) {
        return 0; // Treat uninitialized as 0
    }

    return value;
}

int Menu::saveHighScore(int address, int newTime) {
    // Read the current high score from EEPROM
    int previousRecord = readIntFromEEPROM(address);
    Serial.print("Previous high score: ");
    Serial.println(previousRecord);

    // Check and save new high score
    if (previousRecord == 0 || newTime < previousRecord) {
        EEPROM.write(address, newTime >> 8);  // Write high byte
        EEPROM.write(address + 1, newTime & 0xFF);  // Write low byte
        Serial.println("New high score saved!");
        isNewHighScore = true;
    } else {
        Serial.println("High score not beaten.");
        isNewHighScore = false;
    }

    // Display the current high score
    int savedHighScore = (EEPROM.read(address) << 8) + EEPROM.read(address + 1);
    Serial.print("Current high score: ");
    Serial.println(savedHighScore);
    savedHighScoreSingle = savedHighScore;
    return savedHighScore;
}

void Menu::loadHighScores() {
    savedHighScoreSingle = readIntFromEEPROM(singlePlayerHighScoreAddress);
    savedHighScoreMulti = readIntFromEEPROM(multiPlayerHighScoreAddress);
}
