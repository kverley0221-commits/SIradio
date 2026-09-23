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

    _tx_threshold = radioAPI->get_threshold_value(TX_THRESHOLD_INDEX);
    _rx_threshold = radioAPI->get_threshold_value(RX_THRESHOLD_INDEX);

    fifo_mode = radioAPI->get_global_config(FIFO_MODE_INDEX);
    fifo_mode &= 0x10;
    fifo_mode ? fifo_limit = 128 : fifo_limit = 64;

    return true;
}

/// @brief Send the custom packet typed by user
/// @param msg Serial message typed by user
/// @return If the radio sent the packet or not
void SIradio::sendPacket(String msg)
{
    radioAPI->clear_fifo_cmd(1u);
    radioAPI->clear_interrupts_cmd();
    if (msg.length() >= fifo_limit)
    {
        // Serial.println("Beginning to send long packet");
        msgPtr = msg.c_str();
        payload_index = 0;
        bytes_remaining = msg.length() + 1;
        radioAPI->set_Packet_Length(msg.length());
        radioAPI->write_tx_fifo_cmd(msgPtr, (fifo_limit - 1), true);
        radioAPI->start_tx_cmd(_tx_channel, 0);
        payload_index += (fifo_limit - 1);
        bytes_remaining -= (fifo_limit - 1);
        // Serial.println("Done sending first partition of long packet");
    }
    else
    {
        radioAPI->write_tx_fifo_cmd(msg.c_str(), msg.length(), true);
        radioAPI->start_tx_cmd(_tx_channel, msg.length());
    }
}

bool SIradio::packetSent()
{
    if (index_INT_PH_PEND != -1)
    {
        // Serial.println("Checking FRR for PH_INT");
        frr_reg.frr_values = radioAPI->frr_read_cmd();
        uint8_t int_status = frr_reg.registers[index_INT_PH_PEND];

        if ((int_status & PACKET_SENT_PEND) == PACKET_SENT_PEND)
        {
            radioAPI->clear_interrupts_cmd();
            return true;
        }
        if ((int_status & TX_FIFO_ALMOST_EMPTY_PEND) == TX_FIFO_ALMOST_EMPTY_PEND)
        {
            if (bytes_remaining < _tx_threshold)
                radioAPI->write_tx_fifo_cmd(&msgPtr[payload_index], bytes_remaining, false);
            else
            {
                radioAPI->write_tx_fifo_cmd(&msgPtr[payload_index], _tx_threshold, false);
                bytes_remaining -= _tx_threshold;
                payload_index += _tx_threshold;
            }
            radioAPI->clear_interrupts_cmd();
            return false;
        }
    }
    else if (_nirqPin != -1)
    {
        // Serial.println("Checking NIRQ for PH_INT");
        if (!digitalRead(_nirqPin))
        {
            int_status.interrupt = radioAPI->clear_interrupts_cmd();
            if ((int_status.type.PH_PEND & PACKET_SENT_PEND) == PACKET_SENT_PEND)
                return true;

            if ((int_status.type.PH_PEND & TX_FIFO_ALMOST_EMPTY_PEND) == TX_FIFO_ALMOST_EMPTY_PEND)
            {
                if (bytes_remaining < _tx_threshold)
                    radioAPI->write_tx_fifo_cmd(&msgPtr[payload_index], bytes_remaining, false);
                else
                {
                    radioAPI->write_tx_fifo_cmd(&msgPtr[payload_index], _tx_threshold, false);
                    bytes_remaining -= _tx_threshold;
                    payload_index += _tx_threshold;
                }
                radioAPI->clear_interrupts_cmd();
                return false;
            }
        }
    }
    return false;
}

/// @brief Enter the radio into RX mode
void SIradio::rxMode()
{
    radioAPI->clear_fifo_cmd(2);
    radioAPI->start_rx_cmd(_rx_channel);
}

/// @brief Check to see if the radio received a packet
/// @return If the radio detected a packet or not
// bool SIradio::transmissionDetected()
// {
//     if(!radioAPI->is_fifo_empty())
//     {
//         payload_size = radioAPI->get_payload_size();
//         Serial.println(payload_size);
//         return true;
//     }
//     return false;
// }

// void SIradio::readPacket()
// {
//     // bool int_status;
//     // uint16_t j;
//     uint8_t fifo_limit;
//     uint8_t fifo_mode = radioAPI->get_global_config(FIFO_MODE_INDEX) & 0x10;
//     fifo_mode ? fifo_limit = 128 : fifo_limit = 64;
//     uint16_t total_bytes = payload_size;
//     uint16_t bytes_remaining = total_bytes;
//     uint16_t start_index = 0; // Byte position after sending the size byte and first partition of characters

// }

// /// @brief Have the radio spilt or fuse FIFO buffer
// /// @param enable Determines if the FIFO is split or not
// void SIradio::enable_Split_FIFO(bool enable)
// {
//     // radioAPI->radio_Enable_Split_FIFO(enable);
// }
