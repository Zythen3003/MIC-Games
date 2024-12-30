#include "menu.h"
#include "Nunchuk.h"
#include "GameMechanics.h"
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
                gameStarted = true; // Mark game as started
                SetupGrid(); // Call SetupGrid() for the game
                // Add game logic for singleplayer mode here
            } else if (selectedOption == 1) { // Multiplayer selected
                Serial.println("Multiplayer selected");
                gameStarted = false; // Mark game as started
                tft->fillScreen(ILI9341_DARKGREEN);
                tft->setTextColor(ILI9341_BLACK);
                tft->setTextSize(3);
                tft->setCursor(30, 20);
                tft->println("Multiplayer mode");
                // Add multiplayer setup code here
            }
        }
    }
}

// Constructor
Menu::Menu(Adafruit_ILI9341* tft) {
    this->tft = tft;
    this->selectedOption = 0; // Default to Singleplayer
}

// Draws the entire menu
void Menu::drawMenu() {
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
    unsigned long currentTime = 10;
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
    tft->print(currentTime);

    // Draw the "Highscore" header 
    tft->setTextColor(ILI9341_WHITE);
    tft->setTextSize(2);
    tft->setCursor(xPos + 20, 160);
    tft->print("Highscore");

}


void Menu::displayEndGameMessage() {
    tft->fillScreen(ILI9341_DARKGREEN); // Clear the screen

    tft->setCursor(SCREEN_WIDTH / 2 - 60, SCREEN_HEIGHT / 2 - 40);
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

    tft->setCursor(10, SCREEN_HEIGHT / 2);
    tft->setTextSize(1);
    tft->print("Press button to return to Main Menu...");

     // Reset the game state to prepare for the next game or menu
    player1Score = 0;
    player2Score = 0;

}
