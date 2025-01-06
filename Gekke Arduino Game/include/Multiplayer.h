#ifndef MULTIPLAYER_H
#define MULTIPLAYER_H

#include <stdint.h>
// #include <Infrarood.h>

extern uint8_t player2Score;
extern uint8_t player2X;
extern uint8_t player2Y;

enum Commands
{
    MoveLeft = 0b00000001,          // 1
    MoveRight = 0b00000010,         // 2
    MoveDown = 0b00000011,          // 3
    MoveUp = 0b00000100,            // 4
    Dig = 0b00000101,               // 5
    JoinGame = 0b00000110,          // 6
    JoinedGameSuccess = 0b00000111, // 7
    ResendLastCommand = 0b00001000, // 8
};

bool processCommand(uint8_t test);

int getPlayer2X();
int getPlayer2Y();
void sendCommand(Commands option);
void setPlayer2Coordinates();

#endif