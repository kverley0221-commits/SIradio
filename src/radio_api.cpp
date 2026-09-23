#include "radio_api.h"
#include "radio_comm.h"

/// @brief Initialize RadioAPI data
/// @param _cs Chip select pin
RadioAPI::RadioAPI(uint8_t _cs)
{
    spiDriver = new SPIDriver(_cs);
}

/// @brief Initialize spi driver and load config
/// @return If loading config was successful or not
bool RadioAPI::begin()
{
    if (!spiDriver->begin())
        return false;
    // return loadConfig();
}

/// @brief
/// @param config
/// @return
bool RadioAPI::load_config(const uint8_t *config)
{
    uint16_t i = 0;
    uint8_t cts, cmdLen;

    while (config[i] != 0x00)
    {
        cmdLen = config[i++]; // Read length AND move to first data byte

        if (cmdLen > 16)
            return false; // Safety check from original source

        for (uint8_t j = 0; j < cmdLen; j++)
        {
            spiBuff[j] = config[i++]; // Copy data AND move 'i' forward
        }

        cts = spiDriver->sendCmdGetResponse(cmdLen, spiBuff, 0, 0);
        if (cts != 0xFF)
            return false;
    }
    clear_interrupts_cmd();
    clear_fifo_cmd(3);
    return true;
}

/// @brief Clears all interrupts of radio chip
uint32_t RadioAPI::clear_interrupts_cmd()
{
    spiBuff[0] = 0x20;
    spiBuff[1] = 0x00;
    spiBuff[2] = 0x00;
    spiBuff[3] = 0x00;

    spiDriver->sendCmdGetResponse(4, spiBuff, 8, spiBuff);

    uint32_t response = (uint32_t)(spiBuff[6] << 24) | (uint32_t)(spiBuff[4] << 16) | (uint32_t)(spiBuff[2] << 8) | (uint32_t)(spiBuff[0]);
    return response;
}

/// @brief Reads TX/RX FIFO data
/// @param clrBit either clears or leaves FIFO data as is
void RadioAPI::clear_fifo_cmd(uint8_t clr_bit)
{
    spiBuff[0] = 0x15;
    spiBuff[1] = clr_bit;

    spiDriver->sendCmd(2, spiBuff);
}

/// @brief Write to the TX FIFO Buffer
/// @param msg Pointer to message created by user
void RadioAPI::write_tx_fifo_cmd(const char *msg, const size_t len, bool include_size)
{
    uint8_t msgBuffer[len + 2];
    msgBuffer[0] = 0x66;
    if(include_size)
    {
        msgBuffer[1] = len;
        memcpy(&msgBuffer[2], msg, len); // Copy characters[0-62] into msgBuffer starting at index 2
        spiDriver->sendCmd(len + 2, msgBuffer);

    }
    else
    {
        memcpy(&msgBuffer[1], msg, len);
        spiDriver->sendCmd(len + 1, msgBuffer);
    }
}

/// @brief API command to enter TX mode and transmit data written in TX FIFO
void RadioAPI::start_tx_cmd(uint8_t channel, uint16_t len)
{

    if(len != 0)
        ++len;
    spiBuff[0] = 0x31;
    spiBuff[1] = channel;
    spiBuff[2] = 0x30;
    spiBuff[3] = (uint8_t)(len >> 8);
    spiBuff[4] = (uint8_t)(len);
    spiBuff[5] = 0x00;
    spiBuff[6] = 0x00;

    spiDriver->sendCmd(7, spiBuff);
}

uint8_t RadioAPI::frr_read_cmd()
{
    spiDriver->readData(0x50, 4, 0, spiBuff);
    // for(int i=0; i<4; i++)
    //     Serial.println(spiBuff[i]);
    uint32_t response = (uint32_t)(spiBuff[3] << 24) | (uint32_t)(spiBuff[2] << 16) | (uint32_t)(spiBuff[1] << 8) | (uint32_t)(spiBuff[0]);

    return response;
}

uint8_t RadioAPI::get_threshold_value(uint8_t index)
{
    get_property(0x12, index, 1, spiBuff);
    return spiBuff[0];
}

uint8_t RadioAPI::get_global_config(uint8_t index)
{
    get_property(0x00, index, 1, spiBuff);
    return spiBuff[0];
}

void RadioAPI::set_Packet_Length(uint16_t pkt_size)
{
    ++pkt_size; // Accomodating for the first byte in the packet (How long is the payload)
    spiBuff[0] = (uint8_t)((pkt_size) >> 8);
    spiBuff[1] = (uint8_t)(pkt_size);
    set_property(0x12, 2, 0x0D, spiBuff);
}

/// @brief API command to enter RX mode
void RadioAPI::start_rx_cmd(uint8_t channel)
{
    spiBuff[0] = 0x32;
    spiBuff[1] = channel;
    spiBuff[2] = 0x00;
    spiBuff[3] = 0x00;
    spiBuff[4] = 0x00;
    spiBuff[5] = 0x08;
    spiBuff[6] = 0x08;
    spiBuff[7] = 0x08;

    spiDriver->sendCmd(8, spiBuff);
}

bool RadioAPI::is_fifo_empty()
{
    spiBuff[0] = 0x15;
    spiBuff[1] = 0u;

    spiDriver->sendCmdGetResponse(2, spiBuff, 2, spiBuff);
    return spiBuff[0] == 0;
}

/// @brief Read data stored in RX FIFO
/// @param msg Array stored with the RX FIFO data
void RadioAPI::read_rx_fifo_cmd(const char *msg, size_t len)
{
    spiDriver->readData(0x77, len, 0, (uint8_t*)msg);
}

uint16_t RadioAPI::get_payload_size()
{
    spiDriver->readData(0x77, 1, 0, spiBuff);
    uint16_t payload_size = spiBuff[0];
    Serial.println(payload_size);
    return spiBuff[0];
}

/// @brief API command to configure certain properties of the radio
/// @param grp byte value specifing group
/// @param numProps number of properties to be configured
/// @param index where to start configuring properties
/// @param data Array of data to configure radio
void RadioAPI::set_property(uint8_t grp, uint8_t numProps, uint8_t index, uint8_t *data)
{
    spiBuff[0] = 0x11;
    spiBuff[1] = grp;
    spiBuff[2] = numProps;
    spiBuff[3] = index;
    memcpy(&spiBuff[4], data, numProps);

    spiDriver->sendCmd(4 + numProps, spiBuff);
}

void RadioAPI::get_property(uint8_t grp, uint8_t index, uint8_t numProps, uint8_t *data)
{
    spiBuff[0] = 0x12;
    spiBuff[1] = grp;
    spiBuff[2] = numProps;
    spiBuff[3] = index;
    memcpy(&spiBuff[4], data, numProps);

    spiDriver->sendCmdGetResponse(4 + numProps, spiBuff, numProps, spiBuff);
}