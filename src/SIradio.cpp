#include "SIradio.h"
#include "additional_data.h"
#include "radio_config_rx.h"

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

    // radioAPI->set_Field_Length_1(2u);

    return true;
}

/// @brief Send the custom packet typed by user
/// @param msg Serial message typed by user
/// @return If the radio sent the packet or not
void SIradio::sendPacket(String msg)
{
    // Prepare to enter TX mode and wrtie to the TX FIFO
    radioAPI->read_fifo_cmd(1u);
    radioAPI->clear_interrupts_cmd();

    payload_index = 0;
    bytes_remaining = (uint16_t)(msg.length() + 2); // +2 for the size bytes

    msgPtr[0] = (uint8_t)(bytes_remaining >> 8);
    msgPtr[1] = (uint8_t)(bytes_remaining);
    memcpy(&msgPtr[2], msg.c_str(), msg.length());

    if (bytes_remaining > fifo_limit)
    {
        radioAPI->set_Field_Length_1(bytes_remaining);
        radioAPI->write_tx_fifo_cmd(msgPtr, fifo_limit);
        radioAPI->start_tx_cmd(_tx_channel, 0);
        payload_index += fifo_limit;
        bytes_remaining -= fifo_limit;
    }
    else
    {
        radioAPI->write_tx_fifo_cmd(msgPtr, bytes_remaining);
        radioAPI->start_tx_cmd(_tx_channel, bytes_remaining);
    }
}

bool SIradio::packetSent()
{
    if (index_INT_PH_PEND != -1)
    {
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
                radioAPI->write_tx_fifo_cmd(&msgPtr[payload_index], bytes_remaining);
            else
            {
                radioAPI->write_tx_fifo_cmd(&msgPtr[payload_index], _tx_threshold);
                bytes_remaining -= _tx_threshold;
                payload_index += _tx_threshold;
            }
            radioAPI->clear_interrupts_cmd();
            return false;
        }
    }
    else if (_nirqPin != -1)
    {
        if (!digitalRead(_nirqPin))
        {
            int_status.interrupt = radioAPI->clear_interrupts_cmd();
            if ((int_status.type.PH_PEND & PACKET_SENT_PEND) == PACKET_SENT_PEND)
                return true;

            if ((int_status.type.PH_PEND & TX_FIFO_ALMOST_EMPTY_PEND) == TX_FIFO_ALMOST_EMPTY_PEND)
            {
                if (bytes_remaining < _tx_threshold)
                    radioAPI->write_tx_fifo_cmd((uint8_t *)&msgPtr[payload_index], bytes_remaining);
                else
                {
                    radioAPI->write_tx_fifo_cmd((uint8_t *)&msgPtr[payload_index], _tx_threshold);
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

void SIradio::rxMode()
{
    payload_index = 0;
    pkt_size_found = false;
    radioAPI->read_fifo_cmd(2u);
    radioAPI->clear_interrupts_cmd();
    radioAPI->start_rx_cmd(_rx_channel);
}

bool SIradio::packetReceived()
{

    if (index_INT_PH_PEND != -1)
    {
        frr_reg.frr_values = radioAPI->frr_read_cmd();
        uint8_t int_status = frr_reg.registers[index_INT_PH_PEND];

        if ((int_status & PACKET_RX_PEND) == PACKET_RX_PEND)
        {
            uint8_t last_bytes = radioAPI->read_fifo_cmd(0u);
            radioAPI->read_rx_fifo_cmd(&msgPtr[payload_index], last_bytes);
            if (!pkt_size_found)
            {
                payload_size = ((uint16_t)msgPtr[0] << 8) | (uint16_t)msgPtr[1];
                payload_size -= 2;
            }
            pkt_size_found = false;
            payload_index = 0;
            radioAPI->read_fifo_cmd(2u);
            radioAPI->clear_interrupts_cmd();
            return true;
        }
        if ((int_status & RX_FIFO_ALMOST_FULL_PEND) == RX_FIFO_ALMOST_FULL_PEND)
        {
            radioAPI->read_rx_fifo_cmd(&msgPtr[payload_index], _rx_threshold);
            if (!pkt_size_found)
            {
                payload_size = ((uint16_t)msgPtr[0] << 8) | (uint16_t)msgPtr[1];
                payload_size -= 2;
                pkt_size_found = true;
            }
            payload_index += _rx_threshold;
            radioAPI->clear_interrupts_cmd();
            return false;
        }
    }
    else if (_nirqPin != -1)
    {
        if (!digitalRead(_nirqPin))
        {
            int_status.interrupt = radioAPI->clear_interrupts_cmd();
            if ((int_status.type.PH_PEND & PACKET_RX_PEND) == PACKET_RX_PEND)
            {
                uint8_t last_bytes = radioAPI->read_fifo_cmd(0u);
                radioAPI->read_rx_fifo_cmd(&msgPtr[payload_index], last_bytes);
                if (!pkt_size_found)
                {
                    payload_size = ((uint16_t)msgPtr[0] << 8) | (uint16_t)msgPtr[1];
                    payload_size -= 2;
                }
                pkt_size_found = false;
                payload_index = 0;
                radioAPI->read_fifo_cmd(2u);
                radioAPI->clear_interrupts_cmd();
                return true;
            }
            if ((int_status.type.PH_PEND & RX_FIFO_ALMOST_FULL_PEND) == RX_FIFO_ALMOST_FULL_PEND)
            {
                radioAPI->read_rx_fifo_cmd(&msgPtr[payload_index], _rx_threshold);
                if (!pkt_size_found)
                {
                    payload_size = ((uint16_t)msgPtr[0] << 8) | (uint16_t)msgPtr[1];
                    payload_size -= 2;
                    pkt_size_found = true;
                }
                payload_index += _rx_threshold;
                radioAPI->clear_interrupts_cmd();
                return false;
            }
        }
    }
    return false;
}

void SIradio::getMsg(String &msg)
{
    for (int i = 0; i < payload_size; i++)
        msg += (char)msgPtr[i + 2];
}

// /// @brief Have the radio spilt or fuse FIFO buffer
// /// @param enable Determines if the FIFO is split or not
// void SIradio::enable_Split_FIFO(bool enable)
// {
//     // radioAPI->radio_Enable_Split_FIFO(enable);
// }
