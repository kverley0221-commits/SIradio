#pragma once

#include "radio_api.h"

class SIradio
{
private:
    RadioAPI *radioAPI = NULL;
    uint8_t _sdnPin, _nirqPin, int_Stat, frr_index;
    char msg[128];
    uint8_t frr_modes[4];
    inline bool radio_Poll_INT_Stats(uint8_t int_Group, uint8_t int_Bit);
    inline bool found_FRR_With_Desired_Mode(uint8_t mode);
    inline bool radio_FRR_INT_Stats(uint8_t int_Bit);

public:
    SIradio(uint8_t _sdn, uint8_t _cs, uint8_t nirq);
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
        Serial.println(msg);
        memset(msg, 0, sizeof(msg));
    }
};