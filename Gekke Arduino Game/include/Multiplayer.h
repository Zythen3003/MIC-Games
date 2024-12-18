#ifndef MULTIPLAYER_H
#define MULTIPLAYER_H

#include <stdint.h>
// #include <Infrarood.h>

extern int player2Score;

enum Commands {
    MoveLeft = 0b00000001,
    MoveRight = 0b00000010,
    MoveDown = 0b00000011,
    MoveUp = 0b00000100,
    EnemieDig = 0b00000101,
};

void processCommand(uint8_t test);

int getPlayer2X();
int getPlayer2Y();

#endif