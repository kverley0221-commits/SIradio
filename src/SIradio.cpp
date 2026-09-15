#include "SIradio.h"
#include "additional_data.h"

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

    if (!radioAPI->loadConfig())
        return false;

    radioAPI->get_FRR_Mode(frr_modes);

    return true;
}

/// @brief Set the radio channel number to transmit on 
/// @param TX_Channel 
void SIradio::set_TX_Channel(uint8_t TX_Channel)
{
    radioAPI->radio_Set_TX_Channel(TX_Channel);
}

/// @brief Set the radio channel number to receive on 
/// @param RX_Channel 
void SIradio::set_RX_Channel(uint8_t RX_Channel)
{
    radioAPI->radio_Set_RX_Channel(RX_Channel);
}

/// @brief Parse through FRRs to find one with the desired mode
/// @param mode Desired mode
/// @return If an FRR was found with the desired mode
bool SIradio::found_FRR_With_Desired_Mode(uint8_t mode)
{
    for (int i = 0; i < 4; i++)
    {
        if (frr_modes[i] == mode)
        {
            frr_index = i;
            return true;
        }
    }
    return false;
}

/// @brief Poll the radio interrupt
/// @param int_Group Which group of interrupts to process
/// @param int_Bit Interrupt bit we care for
/// @return If the bit is set high or low
bool SIradio::radio_Poll_INT_Stats(uint8_t int_Group, uint8_t int_Bit)
{
    uint16_t i = 0;
    while (digitalRead(_nirqPin) && (i++ < 500))
        delayMicroseconds(100);
    int_Stat = radioAPI->read_Interrupts(int_Group);
    return (int_Stat & int_Bit) == int_Bit;
}

/// @brief Process interrutps via FRR
/// @param int_Bit Interrupt bit we care for
/// @return If the bit is set high or low
bool SIradio::radio_FRR_INT_Stats(uint8_t int_Bit)
{
    int_Stat = radioAPI->get_FRR_Data(frr_index);
    return (int_Stat & int_Bit) == int_Bit;
}

/// @brief Send the fixed-packet found in config header file
/// @return If the radio sent the packet or not
bool SIradio::send_Fixed_Packet()
{
    radioAPI->radio_Start_TX();
    if (found_FRR_With_Desired_Mode((uint8_t)FRR_MODES::INT_PH_PEND))
        return radio_FRR_INT_Stats(PACKET_SENT_PEND);
    return radio_Poll_INT_Stats((uint8_t)INT_GROUPS::PH, PACKET_SENT_PEND);
}

/// @brief Send the custom packet typed by user
/// @param msg Serial message typed by user
/// @return If the radio sent the packet or not
bool SIradio::sendMessage(String msg)
{
    radioAPI->radio_Start_TX(msg.c_str());
    if (found_FRR_With_Desired_Mode((uint8_t)FRR_MODES::INT_PH_PEND))
    {
        delay(20);
        return radio_FRR_INT_Stats(PACKET_SENT_PEND);
    }
    return radio_Poll_INT_Stats((uint8_t)INT_GROUPS::PH, PACKET_SENT_PEND);
}

/// @brief Enter the radio into RX mode
void SIradio::radio_RX_Mode()
{
    memset(msg, 0, sizeof(msg));
    radioAPI->radio_Start_RX();
}

/// @brief Check to see if the radio received a packet
/// @return If the radio detected a packet or not
bool SIradio::check_Received_Packet()
{
    bool pkt_Recevied = false;
    if (found_FRR_With_Desired_Mode((uint8_t)FRR_MODES::INT_PH_PEND))
    {
        if (radio_FRR_INT_Stats(PACKET_RX_PEND))
            pkt_Recevied = true;
    }
    else if (radio_Poll_INT_Stats((uint8_t)INT_GROUPS::PH, PACKET_RX_PEND))
    {
        pkt_Recevied = true;
    }
    if (pkt_Recevied)
    {
        radioAPI->read_RX_FIFO(msg);
        return true;
    }
    return false;
}

/// @brief Have the radio spilt or fuse FIFO buffer
/// @param enable Determines if the FIFO is split or not
void SIradio::enable_Split_FIFO(bool enable)
{
    radioAPI->radio_Enable_Split_FIFO(enable);
}
