/**
 *******************************************************************************
 * @file    gamepad.h
 * @brief   简要描述
 *******************************************************************************
 * @attention
 *
 * none
 *
 *******************************************************************************
 * @note
 *
 * none
 *
 *******************************************************************************
 * @author  MekLi
 * @date    2025/12/12
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#ifndef PLAY_STATION_GAMEPAD_H
#define PLAY_STATION_GAMEPAD_H




/*-------- 1. includes and imports -----------------------------------------------------------------------------------*/

#include <cstdint>
#include <sys/cdefs.h>



/*-------- 2. enum and define ----------------------------------------------------------------------------------------*/




/*-------- 3. interface ----------------------------------------------------------------------------------------------*/

class Gamepad {
  public:
    Gamepad() = default;

    void* getBufferPointer()const { return (void*)&_gamePadRaw;};

    int16_t getLeftStickX() const { return 0x8000 - _gamePadRaw.leftJoyStickX; }
    int16_t getLeftStickY() const { return 0x7FFF - _gamePadRaw.leftJoyStickY; }
    int16_t getRightStickX() const { return 0x8000 - _gamePadRaw.rightJoyStickX; }
    int16_t getRightStickY() const { return 0x7FFF - _gamePadRaw.rightJoyStickY; }
    bool getUp() const { return _gamePadRaw.dpad == 1; }
    bool getRightUp() const { return _gamePadRaw.dpad == 2; }
    bool getRight() const { return _gamePadRaw.dpad == 3; }
    bool getRightDown() const { return _gamePadRaw.dpad == 4; }
    bool getDown() const { return _gamePadRaw.dpad == 5; }
    bool getLeftDown() const { return _gamePadRaw.dpad == 6; }
    bool getLeft() const { return _gamePadRaw.dpad == 7; }
    bool getLeftUp() const { return _gamePadRaw.dpad == 8; }
    int16_t getLT() { return _gamePadRaw.leftTrigger;}
    int16_t getRT() { return _gamePadRaw.rightTrigger; }
    bool getLB() { return _gamePadRaw.L1;}
    bool getRB() { return _gamePadRaw.R1;}
    bool getA() { return _gamePadRaw.buttonA;}
    bool getB() {return _gamePadRaw.buttonB;}
    bool getX() {return _gamePadRaw.buttonX;}
    bool getY() {return _gamePadRaw.buttonY;}





  private:
    __packed struct {
        uint16_t leftJoyStickX;
        uint16_t leftJoyStickY;
        uint16_t rightJoyStickX;
        uint16_t rightJoyStickY;

        uint16_t leftTrigger; // 最大0x03FF
        uint16_t rightTrigger;

        uint8_t dpad;
        uint32_t buttonA       : 1;
        uint32_t buttonB       : 1;
        uint32_t reverse1  : 1;
        uint32_t buttonY  : 1;

        uint32_t buttonX  : 1;
        uint32_t reverse2 : 1;
        uint32_t L1  : 1;
        uint32_t R1  : 1;

        uint32_t reverse3 : 1;
        uint32_t rPress   : 1;
        uint32_t lPress   : 1;
        uint32_t reverse4 : 4;

        uint32_t Home     : 1;

        uint32_t menu     : 1;
        uint32_t capture  : 1;
        uint32_t reverse5 : 5;
        uint32_t start    : 1;
    } _gamePadRaw = {
        .leftJoyStickX  = 0x8000,
        .leftJoyStickY  = 0x7FFF,
        .rightJoyStickX = 0x8000,
        .rightJoyStickY = 0x7FFF,
    };
};



/*-------- 4. decorator ----------------------------------------------------------------------------------------------*/




/*-------- 5. factories ----------------------------------------------------------------------------------------------*/





#endif /*PLAY_STATION_GAMEPAD_H*/
