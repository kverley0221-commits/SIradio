#pragma once

#include "radio_api.h"

class SIradio
{
private:
    RadioAPI *radioAPI = NULL;
    uint8_t _sdnPin;
    uint8_t _tx_channel = 0;
    uint8_t _rx_channel = 0;
    int _nirqPin = -1;
    int index_INT_PH_PEND = -1; 
    int index_INT_MODEM_PEND = -1;
    int  index_INT_CHIP_PEND = -1;
    int index_LATCHED_RSSI = -1;
    inline bool poll_int_status(uint8_t int_filter);

public:
    SIradio(uint8_t _sdn, uint8_t _cs, uint8_t nirq);
    SIradio(uint8_t _sdn, uint8_t _cs);
    bool begin();
    bool sendPacket(String msg);
    bool receivedPacket();
    void rxMode();


    // void printMsg()
    // {
    //     Serial.println(msgBuffer);
    //     memset(msgBuffer, 0, sizeof(msgBuffer));
    // }
};