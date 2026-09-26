#pragma once

#include "radio_comm.h"

class RadioAPI
{
private:
    SPIDriver *spiDriver = NULL;
    uint8_t spiBuff[16];

    inline void set_property(uint8_t grp, uint8_t index, uint8_t numProps, uint8_t* data);
    inline void get_property(uint8_t grp, uint8_t index, uint8_t numProps, uint8_t* data);
    
    public:
    RadioAPI(uint8_t _cs);
    bool begin();
    bool load_config(const uint8_t* config);
    uint32_t clear_interrupts_cmd();
    uint16_t read_fifo_cmd(uint8_t clr_bit);
    void write_tx_fifo_cmd(const uint8_t *msg, const size_t len);
    void start_tx_cmd(uint8_t channel, uint16_t len);
    uint8_t frr_read_cmd();
    uint8_t get_threshold_value(uint8_t index);
    uint8_t get_global_config(uint8_t index);
    void set_Field_Length_1(uint16_t field_size_1);
    void set_Field_Length_2(uint16_t field_size_2);
    // void set_Field_Length_3(uint16_t field_size_3);
    // void set_Field_Length_4(uint16_t field_size_4);
    // void set_Field_Length_5(uint16_t field_size_5);
    void start_rx_cmd(uint8_t channel);
    void read_rx_fifo_cmd(const uint8_t* msg, size_t len);
    // void radio_Enable_Split_FIFO(bool enable_Bit);
};