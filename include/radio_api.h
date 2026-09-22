#pragma once

#include "radio_comm.h"

class RadioAPI
{
private:
    SPIDriver *spiDriver = NULL;
    uint8_t spiBuff[16];

    inline void start_RX_Cmd();
    inline void get_RX_FIFO_Count();
    inline void set_property(uint8_t grp, uint8_t index, uint8_t numProps, uint8_t* data);
    inline void get_property(uint8_t grp, uint8_t index, uint8_t numProps, uint8_t* data);
    
    public:
    RadioAPI(uint8_t _cs);
    bool begin();
    bool load_config(const uint8_t* config);
    uint32_t clear_interrupts_cmd();
    void clear_fifo_cmd(uint8_t clr_bit);
    void write_tx_fifo_cmd(const char *msg, const size_t len, bool include_size);
    void start_tx_cmd(uint8_t channel, uint16_t len);
    uint8_t frr_read_cmd();
    uint8_t get_threshold_value(uint8_t index);
    uint8_t get_global_config(uint8_t index);
    void set_Packet_Length(uint16_t pkt_size);
    // void radio_Start_TX();
    // void radio_Start_TX(const char* msg);
    // void radio_Start_RX();
    // void read_RX_FIFO(const char* msg);
    // void radio_Enable_Split_FIFO(bool enable_Bit);
};