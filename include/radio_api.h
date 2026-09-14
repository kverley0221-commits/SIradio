#pragma once

#include "radio_comm.h"

class RadioAPI
{
private:
    SPIDriver *spiDriver = NULL;
    uint8_t spiBuff[16];
    uint8_t msgWrite[128];
    uint8_t pktSize;
    uint8_t tx_Channel;
    uint8_t rx_Channel;

    inline void clearFifo(uint8_t clrBit);
    inline void writeTXFifo(const char *msg);
    inline void clearInterrupts();
    inline void start_TX_Cmd();
    inline void start_RX_Cmd();
    inline void get_RX_FIFO_Count();
    inline void set_Property(uint8_t grp, uint8_t index, uint8_t numProps, uint8_t* data);
    
    public:
    RadioAPI(uint8_t _cs);
    bool begin();
    bool loadConfig();
    void radio_Start_TX();
    void radio_Start_TX(const char* msg);
    void radio_Start_RX();
    void read_RX_FIFO(const char* msg);
    void radio_Set_TX_Channel(uint8_t TX_Channel);
    void radio_Set_RX_Channel(uint8_t RX_Channel);
    uint8_t read_Interrupts(uint8_t grp);
    void radio_Enable_Split_FIFO(bool enable_Bit);
    void get_FRR_Mode(uint8_t* modes);
    uint8_t get_FRR_Data(uint8_t index);
};