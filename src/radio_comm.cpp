#include "radio_comm.h"

SPIDriver::SPIDriver(uint8_t _cs)
{
    _csPin = _cs;
}

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

uint8_t SPIDriver::pollCTS()
{
    return getResponse(0, 0);
}

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

uint8_t SPIDriver::sendCmdGetResponse(uint8_t cmdByteCount, uint8_t *pCmdData, uint8_t respByteCount, uint8_t *pRespData)
{
    sendCmd(cmdByteCount, pCmdData);

    return getResponse(respByteCount, pRespData);
}

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
