#pragma once

#include <Arduino.h>
#include <SPI.h>

#define csHigh() (digitalWrite(_csPin, HIGH))
#define csLow() (digitalWrite(_csPin, LOW))
#define spiwrite(data) (_spi->transfer(data))

class SPIDriver
{
private:
    uint8_t _csPin; 
    SPIClass *_spi = NULL;
    bool ctsFlag;
public:
    SPIDriver(uint8_t _cs);
    bool begin();
    void sendCmd(uint8_t byteCount, uint8_t *pData);
    void readData(uint8_t cmd, uint8_t byteCount, uint8_t pollCts, uint8_t *pData);
    uint8_t pollCTS();
    uint8_t getResponse(uint8_t byteCount, uint8_t *pData);
    uint8_t sendCmdGetResponse(uint8_t cmdByteCount, uint8_t *pCmdData, uint8_t respByteCount, uint8_t *pRespData);
};

