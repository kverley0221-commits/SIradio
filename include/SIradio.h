#pragma once

#include "radio_api.h"

class SIradio
{
private:
    RadioAPI *radioAPI = NULL;
    uint8_t _sdnPin, int_Stat, frr_index;
    int _nirqPin = -1;
    int index_INT_PH_PEND = -1, index_INT_MODEM_PEND = -1, index_INT_CHIP_PEND = -1, index_LATCHED_RSSI = -1;
    uint8_t fifoSize = 64;
    char msgBuffer[200];
    uint8_t frr_modes[4];
    inline bool radio_Poll_INT_Stats(uint8_t int_Group, uint8_t int_Bit);
    // inline bool found_FRR_With_Desired_Mode(uint8_t mode);
    inline bool radio_FRR_INT_Stats(uint8_t int_Bit);

public:
    SIradio(uint8_t _sdn, uint8_t _cs, uint8_t nirq);
    SIradio(uint8_t _sdn, uint8_t _cs);
    bool begin();
    void set_TX_Channel(uint8_t TX_Channel);
    void set_RX_Channel(uint8_t RX_Channel);
    bool send_Fixed_Packet();
    bool check_Received_Packet();
    void radio_RX_Mode();
    bool sendMessage(String msg);
    void enable_Split_FIFO(bool enable);
    void printMsg()
    {
        Serial.println(msgBuffer);
        memset(msgBuffer, 0, sizeof(msgBuffer));
    }
};