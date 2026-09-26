#pragma once

#include "radio_api.h"

class SIradio
{
private:
    RadioAPI *radioAPI = NULL;
    uint8_t _sdnPin;
    uint8_t _tx_channel = 0;
    uint8_t _rx_channel = 0;
    uint16_t payload_index;
    uint16_t payload_size;
    uint8_t _tx_threshold;
    uint8_t _rx_threshold;
    uint16_t bytes_remaining;
    uint16_t strLen;
    uint8_t fifo_limit;
    bool fifo_mode;
    bool pkt_size_found = false;

    uint8_t msgPtr[400];
    int _nirqPin = -1;
    int index_INT_PH_PEND = -1; 
    int index_INT_MODEM_PEND = -1;
    int  index_INT_CHIP_PEND = -1;
    int index_LATCHED_RSSI = -1;

public:
    SIradio(uint8_t _sdn, uint8_t _cs, uint8_t nirq);
    SIradio(uint8_t _sdn, uint8_t _cs);
    bool begin();
    void sendPacket(String msg);
    bool packetSent();
    void rxMode();
    bool packetReceived();

    void getMsg(String &msg);
};