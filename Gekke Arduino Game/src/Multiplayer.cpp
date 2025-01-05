#include <Multiplayer.h>
#include <HardwareSerial.h>
#include <GameMechanics.h>
#include <Infrarood.h>
#include <GenericStack.h>

uint8_t player2Score;
uint8_t player2X;
uint8_t player2Y;

uint16_t lastFallingEdgeTime = 0;

GenericStack<Commands, 5> commandStack;

bool isLeader = false;

bool processCommand(uint8_t test)
{
    switch (test)
    {
    case MoveLeft:
        updateCell(false, player2X, player2Y);
        if (player2X != 0)
        {
            player2X--;
        }
        updateCell(false);
        break;
    case MoveRight:
        updateCell(false, player2X, player2Y);
        if (player2X != GRID_SIZE - 1)
        {
            player2X++;
        }
        updateCell(false);
        break;
    case MoveDown:
        updateCell(false, player2X, player2Y);
        if (player2Y != GRID_SIZE - 1)
        {
            player2Y++;
        }
        updateCell(false);
        break;
    case MoveUp:
        updateCell(false, player2X, player2Y);
        if (player2Y != 0)
        {
            player2Y--;
        }
        updateCell(false);
        break;
    case Dig:
        // Add cooldown
        digAction(false);
        break;
    case JoinGame:
        isLeader = true;
        sendCommand(JoinedGameSuccess);
        setPlayer2Coordinates();
        return true;
        break;
    case JoinedGameSuccess:
        isLeader = false;
        setPlayer2Coordinates();
        return true;
        break;
        break;
    case ResendLastCommand:
        Commands command;
        if (commandStack.peek(command)) {
            sendCommand(command);
        } else {
            // Serial.println("VERY BAD ERROR, NO COMMANDS ON STACK");
        }
        break;
    default:
        // Serial.println("Unknown Command, sending resend command");
        sendCommand(ResendLastCommand);
        break;
    }

    return false;
}

ISR(INT0_vect)
{
  uint16_t pulseDuration = readCount - lastFallingEdgeTime;
  lastFallingEdgeTime = readCount;

  uint16_t nextHead = (bufferHead + 1) % BUFFER_SIZE1;
  if (nextHead == bufferTail)
  {
    bufferOverflow = true;
  }
  else
  {
    pulseBuffer[bufferHead] = pulseDuration;
    bufferHead = nextHead;
    newPulseAvailable = true;
  }
}

void sendCommand(Commands option)
{
    commandStack.push(option);
    sendDataByte(option);
}

int getPlayer2X()
{
    return player2X;
}

int getPlayer2Y()
{
    return player2Y;
}

void setPlayer2Coordinates()
{
    if (isLeader)
        player2X = player2Y = GRID_SIZE - 1;
    else
        player2X = player2Y = 0;

    setPlayer1Coordinates(isLeader);
}