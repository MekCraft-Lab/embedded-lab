/**
 *******************************************************************************
 * @file    w25qxx.h
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
 * @date    2025/8/24
 * @version 1.0
 *******************************************************************************
 */


/* Define to prevent recursive inclusion -----------------------------------------------------------------------------*/

#pragma once
#include "w25qxx-inst.h"

#include <cstdint>
#include <vector>


/*-------- includes --------------------------------------------------------------------------------------------------*/





/*-------- class prototypes-------------------------------------------------------------------------------------------*/

enum class W25QxxErr {
    NONE = 0,
    BUSY,
    TIMEOUT,
    INVALID,
    SUCCESS,
};

enum class W25QxxCommModeEnum {
    NONE,
    STANDARD_SPI,
    DSPI,
    QSPI,
    OSPI,
};


enum class MsgTypeEnum {
    NONE,
    INSTRUCTION,
    ADDRESS,
    ALTERNATE_BYTE,
    DUMMY_CYCLE,
    DATA,
};

enum class W25QxxRegisterEnum {
    STATUS_REGISTER_1,
    STATUS_REGISTER_2,
    STATUS_REGISTER_3,
    SECURITY_REGISTER_1,
    SECURITY_REGISTER_2,
    SECURITY_REGISTER_3,
};

enum class W25QxxWrapLenEnum {
    NONE,
    WRAP_8_BYTE,
    WRAP_16_BYTE,
    WRAP_32_BYTE,
    WRAP_64_BYTE,
};

enum class W25QxxStateEnum {
    NONE,
    FREE,
    BUSY,
    SUSPEND,
};



class W25QxxMsgElement {

  public:
    /**
     * @brief Initialize of address, alternate bytes, dummy-cycle
     * @param type Data type
     * @param byte 32bit
     * @param num number of bytes
     * @param mode lines
     */
    W25QxxMsgElement(const MsgTypeEnum type, const uint32_t byte, const uint16_t num, const W25QxxCommModeEnum mode)
        : _type(type), _mode(mode), _num(num), _byte(byte) {
        if (type == MsgTypeEnum::DATA || type == MsgTypeEnum::INSTRUCTION) {
            _type = MsgTypeEnum::NONE;
        }
    }

    /**
     * @brief initialize of data
     * @param type DATA
     * @param pData pointer of data
     * @param num length
     * @param mode lines
     */
    W25QxxMsgElement(const MsgTypeEnum type, uint8_t* pData, const uint16_t num, const W25QxxCommModeEnum mode)
        : _type(type), _mode(mode), _num(num), _pData(pData) {
        if (type != MsgTypeEnum::DATA) {
            _type = MsgTypeEnum::NONE;
        }
    }

    /**
     * @brief initialize of instruction
     * @param type INSTRUCTION
     * @param inst content
     */
    W25QxxMsgElement(const MsgTypeEnum type, W25QxxInst inst)
        : _type(type), _mode(W25QxxCommModeEnum::STANDARD_SPI), _num(1), _byte(static_cast<uint8_t>(inst)) {
        if (type != MsgTypeEnum::INSTRUCTION) {
            _type = MsgTypeEnum::NONE;
        }
    }

    W25QxxMsgElement(const MsgTypeEnum type, const uint16_t num) : _type(type), _num(num) {
        if (type != MsgTypeEnum::DUMMY_CYCLE || num > 31) {
            _type = MsgTypeEnum::NONE;
        }
    }

    /*********** setter & getter *************/
    [[nodiscard]] MsgTypeEnum getType() const { return _type; }
    [[nodiscard]] uint16_t getNum() const { return _num; }
    [[nodiscard]] uint32_t getByte() const { return _byte; }
    [[nodiscard]] uint8_t* getPData() const { return _pData; }
    [[nodiscard]] W25QxxCommModeEnum getMode() const { return _mode; }
    [[nodiscard]] uint8_t getLines() const { return static_cast<uint8_t>(_mode); }

  private:
    MsgTypeEnum _type = MsgTypeEnum::NONE;
    W25QxxCommModeEnum _mode;
    uint16_t _num;
    uint32_t _byte;
    uint8_t* _pData = nullptr;
};





class W25QxxRxOpt {

  public:
    W25QxxRxOpt(void* pData, const uint16_t len) : _pData(pData), _len(len) {};

    /*********** setter & getter ***********/
    [[nodiscard]] void* getPData() const { return _pData; }
    [[nodiscard]] uint16_t getLen() const { return _len; }

  private:
    void* _pData  = nullptr;
    uint16_t _len = 0;
};




class W25QxxSpiComm {
  public:
    explicit W25QxxSpiComm(void* commHandle) : _commHandle(commHandle) {};
    [[nodiscard]] W25QxxErr transmit(const std::vector<W25QxxMsgElement>&) const;
    [[nodiscard]] W25QxxErr receive(const std::vector<W25QxxMsgElement>& msg) const;

    [[nodiscard]] W25QxxErr autoPolling(std::vector<W25QxxMsgElement>& msgs , uint32_t target, uint32_t mask, uint8_t interval);

private:
    void* _commHandle = nullptr;
};


/**
 * @brief The interface of W25Qxx
 */
class W25Qxx {
public:
    explicit W25Qxx(void* _commHandle) : _commHandle(_commHandle), _spiComm(_commHandle) {};

    /**
     * @brief Send instruction to read the ID of flash.
     * @return W25QxxErr
     */
    W25QxxErr enquireJedecIdAsync();


    /**
     * @brief Send instruction to read the 8 bit ID of flash device.
     * @return W25QxxErr
     */
    W25QxxErr enquireDeviceIdAsync();


    /**
     * @brief Send instruction to read the 64 bit unique ID of flash device.
     * @return W25QxxErr
     */
    W25QxxErr enquireUniqueIdAsync();


    /**
     * @brief ReadRegister
     * @param sr Select the status register want to read.
     * @return Error Code
     */
    W25QxxErr enquireStatusRegisterAsync(W25QxxRegisterEnum sr);


    /**
     * @brief Write Enable
     * @return W25Qxx Error Code
     */
    W25QxxErr writeEnable() const;


    /**
     * @brief Write byte into Registers
     * @param sr Status Register Enum
     * @param byte content
     * @return W25Qxx Error Code
     */
    W25QxxErr writeRegister(const W25QxxRegisterEnum sr, uint8_t byte) const;


    /**
     * @brief Write enable for volatile status register.
     * @return Error Code
     */
    W25QxxErr writeEnableForVolatileStatusRegister() const;


    /**
     * @brief Write disable.
     * @return Error Code
     */
    W25QxxErr writeDisable() const;


    /**
     * @brief read data from flash.
     * @param address 24 bits address
     * @param pData pointer of data
     * @param len length to read
     * @return error code
     */
    W25QxxErr readData(uint32_t address, void* pData, uint16_t len) const;

    W25QxxErr fastReadData(uint32_t address, void* pData, uint16_t len) const;

    W25QxxErr fastReadDualOutput(uint32_t address, void* pData, uint16_t len) const;

    W25QxxErr fastReadQuadOutput(uint32_t address, void* pData, uint16_t len) const;

    W25QxxErr fastReadDualIO(uint32_t address, void* pData, uint16_t len) const;

    W25QxxErr fastReadQuadIO(uint32_t address, void* pData, uint16_t len) const;


    /**
     * @brief set burst with wrap
     * @param len wrap length
     * @return error code
     */
    W25QxxErr setBurstWithWrap(W25QxxWrapLenEnum len) const;



    W25QxxErr pageProgram(uint32_t address, const void *pData, uint16_t len) const;

    W25QxxErr quadInputPageProgram(uint32_t address, const void* pData, uint16_t len) const;



    W25QxxErr sectorErase(uint32_t address) const;

    W25QxxErr blockErase32KB(uint32_t address) const;

    W25QxxErr blockErase64KB(uint32_t address) const;

    W25QxxErr chipErase()const;



    W25QxxErr eraseOrProgramSuspend() const;

    W25QxxErr eraseOrProgramResume() const;




    W25QxxErr powerDown() const;

    W25QxxErr powerOn() const;



    W25QxxErr enquireManDevIDAsync();

    W25QxxErr enquireManDevIDDualIOAsync();

    W25QxxErr enquireManDevIDQuadIOAsync();

    W25QxxErr enquireSfdpRegisterAsync();



    W25QxxErr eraseSecurityRegister(W25QxxRegisterEnum sr) const ;

    W25QxxErr programSecurityRegister(W25QxxRegisterEnum sr, void *pData, uint8_t len) const ;

    W25QxxErr readSecurityRegister(W25QxxRegisterEnum sr, uint8_t address, void *pData, uint8_t len) const;



    W25QxxErr lockBlockOrSector(uint32_t address)const;

    W25QxxErr unlockBlockOrSector(uint32_t address)const;

    W25QxxErr enquireBlockOrSectorLock(uint32_t address, uint8_t *isLocked) const;

    W25QxxErr globalLock()const;

    W25QxxErr globalUnlock()const;


    W25QxxErr resetEnable()const;
    W25QxxErr reset()const;


    W25QxxErr asyncRxCallback();

    W25QxxErr asyncWaitForFlag(W25QxxStateEnum s);


    /************* setter & getter *************/

    [[nodiscard]] W25QxxCommModeEnum getCommMode() const { return _commMode; }
    void setCommMode(const W25QxxCommModeEnum mode) { _commMode = mode; }

    [[nodiscard]] uint8_t getBusyBit() const {return _statusRegister1 & 1;}

    [[nodiscard]] uint8_t getWelBit() const {return _statusRegister1 & (1 << 1);}

    [[nodiscard]] uint8_t getQeBit() const{return _statusRegister2 & (1 << 1);}

    [[nodiscard]] uint8_t getWpsBit() const {return _statusRegister3 & (1 << 2);}

    [[nodiscard]] uint8_t getSusBit() const {return _statusRegister2 & (1 << 7);}

    [[nodiscard]] uint64_t getSFDP() const {return _sfdpRegister;}

    [[nodiscard]] uint8_t getSR1() const {return _statusRegister1;}

    [[nodiscard]] uint8_t getSR2() const {return _statusRegister2;}

    [[nodiscard]] uint8_t getSR3() const {return _statusRegister3;}

    [[nodiscard]] uint8_t getMID() const {return _manufacturerID;}

    [[nodiscard]] uint16_t getDevID16() const{return _deviceID16;}

    [[nodiscard]] uint16_t getDevID8() const{return _deviceID8;}

    [[nodiscard]] uint64_t getUniqueID() const{return _uniqueID;}


    /************* setter & getter *************/


  private:
    uint8_t _manufacturerID      = 0x00;
    uint16_t _deviceID16         = 0x00;
    uint8_t _deviceID8           = 0x00;
    uint64_t _uniqueID           = 0x00;
    uint8_t _pRxBuff[128]        = {};
    uint8_t _statusRegister1     = 0x00;
    uint8_t _statusRegister2     = 0x00;
    uint8_t _statusRegister3     = 0x00;
    uint64_t _sfdpRegister = 0;
    void* _commHandle            = nullptr;
    W25QxxCommModeEnum _commMode = W25QxxCommModeEnum::NONE;
    W25QxxSpiComm _spiComm;
    std::vector<W25QxxRxOpt> _rxOpts;
};



/*-------- define ----------------------------------------------------------------------------------------------------*/


/*-------- macro -----------------------------------------------------------------------------------------------------*/


/*-------- variables -------------------------------------------------------------------------------------------------*/


/*-------- function prototypes ---------------------------------------------------------------------------------------*/