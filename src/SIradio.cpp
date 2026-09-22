#include "SIradio.h"
#include "additional_data.h"
#include "radio_config.h"

static const uint8_t config[] = RADIO_CONFIGURATION_DATA_ARRAY;
static const uint8_t frr_modes[] = {RF_FRR_CTL_A_MODE_4};
static pending_Interrupts int_status;
static frr_registers frr_reg;

/// @brief Initialize SIradio class data
/// @param _sdn Pin for resetting radio
/// @param _cs Chip select pin for spi driver
/// @param _nirq Radio NIRQ pin
SIradio::SIradio(uint8_t _sdn, uint8_t _cs, uint8_t _nirq)
{
    _sdnPin = _sdn;
    _nirqPin = _nirq;
    radioAPI = new RadioAPI(_cs);
    pinMode(_sdnPin, OUTPUT);
    pinMode(_nirqPin, INPUT);
}

/// @brief Initialize SIradio class data
/// @param _sdn Pin for resetting radio
/// @param _cs Chip select pin for spi driver
SIradio::SIradio(uint8_t _sdn, uint8_t _cs)
{
    _sdnPin = _sdn;
    _nirqPin = -1;
    radioAPI = new RadioAPI(_cs);
    pinMode(_sdnPin, OUTPUT);
    pinMode(_nirqPin, INPUT);
}

/// @brief Perform radio hardware reset and config
/// @return Wheather or not the radio successfully configured
bool SIradio::begin()
{

    radioAPI->begin();
    digitalWrite(_sdnPin, LOW);
    delayMicroseconds(30);
    digitalWrite(_sdnPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(_sdnPin, LOW);

    if (!radioAPI->load_config(config))
        return false;

    for (int i = 0; i < 4; i++)
    {
        switch (frr_modes[i + 4])
        {
        case 4:
            index_INT_PH_PEND = i;
            break;
        case 6:
            index_INT_MODEM_PEND = i;
            break;
        case 8:
            index_INT_CHIP_PEND = i;
            break;
        case 10:
            index_LATCHED_RSSI = i;
            break;

        default:
            break;
        }
    }

    return true;
}

/// @brief Send the custom packet typed by user
/// @param msg Serial message typed by user
/// @return If the radio sent the packet or not
bool SIradio::sendPacket(String msg)
{
    uint8_t fifo_limit;
    bool int_status;
    uint16_t j;
    uint8_t fifo_mode = radioAPI->get_global_config(FIFO_MODE_INDEX) & 0x10;
    fifo_mode ? fifo_limit = 128 : fifo_limit = 64;

    if (msg.length() > fifo_limit)
    {
        const char *ptr = msg.c_str();
        const uint8_t threshold = radioAPI->get_threshold_value(TX_THRESHOLD_INDEX);
        uint16_t total_bytes = msg.length();
        uint16_t bytes_remaining = total_bytes;
        uint16_t start_index = 0; // Byte position after sending the size byte and first partition of characters

        radioAPI->set_Packet_Length(msg.length());
        radioAPI->write_tx_fifo_cmd(ptr, fifo_limit - 1, true); // Subtracting 1 so that only the first 63 bytes + the size byte get trasmitted first
        bytes_remaining -= (fifo_limit - 1);
        start_index += (fifo_limit - 1);
        radioAPI->start_tx_cmd(_tx_channel, 0);

        frr_reg.frr_values = radioAPI->frr_read_cmd();
        int_status = (frr_reg.registers[index_INT_PH_PEND] & TX_FIFO_ALMOST_EMPTY_PEND) == TX_FIFO_ALMOST_EMPTY_PEND;
        uint8_t loops = msg.length() / fifo_limit; // How many partions of the message should occur

        for (int i = 0; i < loops; i++)
        {
            j = 0;
            while (!int_status && (j++ < 500))
            {
                frr_reg.frr_values = radioAPI->frr_read_cmd();
                int_status = (frr_reg.registers[index_INT_PH_PEND] & TX_FIFO_ALMOST_EMPTY_PEND) == TX_FIFO_ALMOST_EMPTY_PEND;
                delayMicroseconds(100);
            }
            if (int_status)
            {
                Serial.println("TX FIFO ALMOST EMPTY INTERRUPT TRIGGERED");
                if (bytes_remaining < threshold)
                    radioAPI->write_tx_fifo_cmd(&ptr[start_index], bytes_remaining, false);
                else
                {
                    radioAPI->write_tx_fifo_cmd(&ptr[start_index], threshold, false);
                    bytes_remaining -= threshold;
                    start_index += threshold;
                }
            }
            else
                return false;
            radioAPI->clear_interrupts_cmd();
        }
    }
    Serial.println("Finished sending long packet");

    if (index_INT_PH_PEND != -1)
    {
        j = 0;
        int_status = (frr_reg.registers[index_INT_PH_PEND] & PACKET_SENT_PEND) == PACKET_SENT_PEND;
        while(!int_status && (j++ < 1000))
        {
            delay(10);
            frr_reg.frr_values = radioAPI->frr_read_cmd();
            int_status = (frr_reg.registers[index_INT_PH_PEND] & PACKET_SENT_PEND) == PACKET_SENT_PEND;
        }
        // frr_reg.frr_values = radioAPI->frr_read_cmd();
        radioAPI->clear_interrupts_cmd();
        return int_status;
    }
    else if (_nirqPin != -1)
        return poll_int_status((uint8_t)PACKET_SENT_PEND);

    return false;
}

/// @brief Poll the NIRQ pin for interrupts
/// @param int_Bit Interrupt bit we care for
/// @return If the bit is set high or low
bool SIradio::poll_int_status(uint8_t int_Bit)
{
    uint16_t i = 0;
    while (digitalRead(_nirqPin) && (i++ < 500))
        delayMicroseconds(100);
    int_status.interrupt = radioAPI->clear_interrupts_cmd();
    return (int_status.type.PH_PEND & int_Bit) == int_Bit;
}

// /// @brief Process interrutps via FRR
// /// @param int_Bit Interrupt bit we care for
// /// @return If the bit is set high or low
// bool SIradio::radio_FRR_INT_Stats(uint8_t int_Bit)
// {
//     uint8_t int_Status = radioAPI->get_FRR_Data(frr_index);
//     return (int_Status & int_Bit) == int_Bit;
// }

// /// @brief Enter the radio into RX mode
// void SIradio::radio_RX_Mode()
// {
//     memset(msgBuffer, 0, sizeof(msgBuffer));
//     radioAPI->radio_Start_RX();
// }

// /// @brief Check to see if the radio received a packet
// /// @return If the radio detected a packet or not
// bool SIradio::check_Received_Packet()
// {
//     bool pkt_Recevied = false;
//     if (index_INT_PH_PEND != -1)
//         pkt_Recevied = radio_FRR_INT_Stats(PACKET_RX_PEND);
//     else if (_nirqPin != -1)
//         pkt_Recevied = true;

//     if (pkt_Recevied)
//     {
//         radioAPI->read_RX_FIFO(msgBuffer);
//         return true;
//     }
//     return false;
// }

// /// @brief Have the radio spilt or fuse FIFO buffer
// /// @param enable Determines if the FIFO is split or not
// void SIradio::enable_Split_FIFO(bool enable)
// {
//     // radioAPI->radio_Enable_Split_FIFO(enable);
// }
