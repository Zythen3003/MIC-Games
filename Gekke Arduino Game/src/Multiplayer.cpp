#include <Multiplayer.h>
#include <HardwareSerial.h>
#include <GameMechanics.h>

extern int player2Score;
int player2X;
int player2Y;

void processCommand(uint8_t test) {
    switch (test) {
        case MoveLeft:
            if (player2X != 0) {
                player2X--;
            }
            break;
        case MoveRight:
            if (player2X != GRID_SIZE-1) {
                player2X++;
            }
            break;
        case MoveDown:
            if (player2Y != GRID_SIZE-1) {
                player2Y--;
            }
            break;
        case MoveUp:
            if (player2Y != 0) {
                player2Y--;
            }
            break;
        case EnemieDig:
            // Add cooldown
            digAction(player2X, player2Y, false);
            break;
        default:
            Serial.println("Unknown Command");
            break;
    }


}

int getPlayer2X() {
    return player2X;
}

int getPlayer2Y() {
    return player2Y;
}