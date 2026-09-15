#include "radio_comm.h"

/// @brief Initialize spi driver data
/// @param _cs Chip select pin
SPIDriver::SPIDriver(uint8_t _cs)
{
    _csPin = _cs;
}

/// @brief Initialize and configre parameters for spi driver
/// @return 
bool SPIDriver::begin()
{
    #if defined(ARDUINO_ARCH_RP2040)
    _spi = &SPI;
    #endif
    pinMode(_csPin, OUTPUT);
    
    _spi->begin();
    _spi->beginTransaction(SPISettings(10000000, MSBFIRST, SPI_MODE0));
    ctsFlag = 0;
    return true;
}

/// @brief Send strean of byte commands
/// @param byteCount Number of bytes to send
/// @param pData Array of data for sending
void SPIDriver::sendCmd(uint8_t byteCount, uint8_t *pData)
{
    if (byteCount == 1)
        byteCount++;


    while (!ctsFlag)
    {
        pollCTS();
    }
    csLow();
    while (byteCount--)
    {
        spiwrite(*pData++);
    }
    csHigh();
    ctsFlag = false; // Reset for next command
}

/// @brief Poll for the CTS
/// @return 
uint8_t SPIDriver::pollCTS()
{
    return getResponse(0, 0);
}

/// @brief Read byte response from radio
/// @param byteCount Number of bytes to read
/// @param pData Array to store response
/// @return CTS
uint8_t SPIDriver::getResponse(uint8_t byteCount, uint8_t *pData)
{
    uint8_t ctsVal = 0;
    uint32_t errCnt = 40000;
    while (errCnt != 0)
    {
        csLow();
        _spi->transfer(0x44);
        ctsVal = _spi->transfer(0x00); // MUST be 0x00

        if (ctsVal == 0xFF)
        {
            while (byteCount--)
            {
                *pData++ = _spi->transfer(0x00);
            }
            csHigh(); // End transaction on success
            ctsFlag = true;
            return 0xFF;
        }
        csHigh();              // MUST toggle CS high to reset the 0x44 command
        delayMicroseconds(20); // Give the radio's 8051 core time to react
        errCnt--;
    }
    return 0x00;
}

/// @brief Send byte stream to radio, then read byte stream response
/// @param cmdByteCount Number of bytes to send
/// @param pCmdData Array stored with command data
/// @param respByteCount Number of bytes to read
/// @param pRespData Array to store response data
/// @return Data stream
uint8_t SPIDriver::sendCmdGetResponse(uint8_t cmdByteCount, uint8_t *pCmdData, uint8_t respByteCount, uint8_t *pRespData)
{
    sendCmd(cmdByteCount, pCmdData);

    return getResponse(respByteCount, pRespData);
}

/// @brief Read data stream from radio without polling CTS
/// @param cmd API command
/// @param byteCount Number of bytes to read from radio
/// @param pollCts Poll CTS or not
/// @param pData Array stored with resposne data
void SPIDriver::readData(uint8_t cmd, uint8_t byteCount, uint8_t pollCts, uint8_t *pData)
{
    if(pollCts)
    {
        while(!ctsFlag)
        {
            pollCTS();
        }
    }

    csLow();
    spiwrite(cmd);
    while (byteCount--)
    {
        *pData++ = _spi->transfer(0x00);
    }
    csHigh();
    ctsFlag = false;
}
