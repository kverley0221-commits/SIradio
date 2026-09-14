#include "SIradio.h"
#include "additional_data.h"

SIradio::SIradio(uint8_t _sdn, uint8_t _cs, uint8_t _nirq)
{
    _sdnPin = _sdn;
    _nirqPin = _nirq;
    radioAPI = new RadioAPI(_cs);
    pinMode(_sdnPin, OUTPUT);
    pinMode(_nirqPin, INPUT);
}

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

void SIradio::set_TX_Channel(uint8_t TX_Channel)
{
    radioAPI->radio_Set_TX_Channel(TX_Channel);
}

void SIradio::set_RX_Channel(uint8_t RX_Channel)
{
    radioAPI->radio_Set_RX_Channel(RX_Channel);
}

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

bool SIradio::radio_Poll_INT_Stats(uint8_t int_Group, uint8_t int_Bit)
{
    uint16_t i = 0;
    while (digitalRead(_nirqPin) && (i++ < 500))
        delayMicroseconds(100);
    int_Stat = radioAPI->read_Interrupts(int_Group);
    return (int_Stat & int_Bit) == int_Bit;
}

bool SIradio::radio_FRR_INT_Stats(uint8_t int_Bit)
{
    int_Stat = radioAPI->get_FRR_Data(frr_index);
    return (int_Stat & int_Bit) == int_Bit;
}

bool SIradio::send_Fixed_Packet()
{
    radioAPI->radio_Start_TX();
    if (found_FRR_With_Desired_Mode((uint8_t)FRR_MODES::INT_PH_PEND))
        return radio_FRR_INT_Stats(PACKET_SENT_PEND);
    return radio_Poll_INT_Stats((uint8_t)INT_GROUPS::PH, PACKET_SENT_PEND);
}

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

void SIradio::radio_RX_Mode()
{
    memset(msg, 0, sizeof(msg));
    radioAPI->radio_Start_RX();
}

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

void SIradio::enable_Split_FIFO(bool enable)
{
    radioAPI->radio_Enable_Split_FIFO(enable);
}
