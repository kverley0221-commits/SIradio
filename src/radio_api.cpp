#include "radio_api.h"
// #include "radio_config_tx.h"
#include "radio_config.h"
#include "radio_comm.h"
#include "additional_data.h"

// Define the exact properties we're looking for
#define property_PKT_LEN_FIELD_SOURCE 0x1209 // PKT_LEN
#define property_PKT_LEN 0x1208              // PKT_FIELD_5_LENGTH_7_0

static const uint8_t config[] = RADIO_CONFIGURATION_DATA_ARRAY;
static const uint8_t frr_modes[] = {RF_FRR_CTL_A_MODE_4};

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
    spiDriver->begin();
    return loadConfig();
}

/// @brief loads config header file into radio chip
/// @return true if confid loaded successfully, false if not
bool RadioAPI::loadConfig()
{
    uint16_t i = 0;
    uint8_t buff[20];
    uint8_t cts;
    tx_Channel = RADIO_CONFIGURATION_DATA_CHANNEL_NUMBER;
    rx_Channel = RADIO_CONFIGURATION_DATA_CHANNEL_NUMBER;

    while (config[i] != 0x00)
    {
        uint8_t cmdLen = config[i++]; // Read length AND move to first data byte

        if (cmdLen > 16)
            return false; // Safety check from original source

        for (uint8_t j = 0; j < cmdLen; j++)
        {
            buff[j] = config[i++]; // Copy data AND move 'i' forward
        }

        cts = spiDriver->sendCmdGetResponse(cmdLen, buff, 0, 0);
        if (cts != 0xFF)
            return false;
    }
    clearInterrupts();
    return true;
}

/// @brief Clears all interrupts of radio chip
void RadioAPI::clearInterrupts()
{
    spiBuff[0] = 0x20;
    spiBuff[1] = 0x00;
    spiBuff[2] = 0x00;
    spiBuff[3] = 0x00;

    spiDriver->sendCmd(4, spiBuff);
}

/// @brief Reads TX/RX FIFO data
/// @param clrBit either clears or leaves FIFO data as is
void RadioAPI::clearFifo(uint8_t clrBit)
{
    spiBuff[0] = 0x15;
    spiBuff[1] = clrBit;

    spiDriver->sendCmd(2, spiBuff);
}

/// @brief set the channel for the radio to transmit on
/// @param TX_Channel Radio channel number
void RadioAPI::radio_Set_TX_Channel(uint8_t TX_Channel)
{
    tx_Channel = TX_Channel;
}

/// @brief set the channel for the radio to receive in
/// @param RX_Channel Radio channel number
void RadioAPI::radio_Set_RX_Channel(uint8_t RX_Channel)
{
    rx_Channel = RX_Channel;
}

/// @brief Write to the TX FIFO
/// @param msg Message created by user from Serial
void RadioAPI::writeTXFifo(const char *msg)
{
    msgWrite[0] = 0x66;
    msgWrite[1] = pktSize;
    memcpy(&msgWrite[2], msg, (size_t)pktSize);
    spiDriver->sendCmd(pktSize + 2, msgWrite);
}

/// @brief Read the first byte of data from RX FIFO for the packet size
void RadioAPI::get_RX_FIFO_Count()
{
    spiDriver->readData(0x77, 1, 0, spiBuff);
    pktSize = spiBuff[0];
}

/// @brief API command to enter TX mode and transmit data written in TX FIFO
void RadioAPI::start_TX_Cmd()
{

    spiBuff[0] = 0x31;
    spiBuff[1] = tx_Channel;
    spiBuff[2] = 0x30;
    spiBuff[3] = (uint8_t)((pktSize + 1) >> 8);
    spiBuff[4] = (uint8_t)(pktSize + 1);
    spiBuff[5] = 0x00;
    spiBuff[6] = 0x00;

    spiDriver->sendCmd(7, spiBuff);
}

/// @brief Prepare the radio to enter TX mode and transmit a fixed packet
void RadioAPI::radio_Start_TX()
{
    const char msg[] = RADIO_CONFIGURATION_DATA_CUSTOM_PAYLOAD;
    pktSize = RADIO_CONFIGURATION_DATA_RADIO_PACKET_LENGTH;
    clearInterrupts();
    writeTXFifo(msg);
    start_TX_Cmd();
}

/// @brief Prepare the radio to enter TX mode and transmit a custom packet
void RadioAPI::radio_Start_TX(const char *msg)
{
    pktSize = strlen(msg);
    clearInterrupts();
    clearFifo(0X01);
    writeTXFifo(msg);
    start_TX_Cmd();
}

/// @brief API command to enter RX mode
void RadioAPI::start_RX_Cmd()
{
    spiBuff[0] = 0x32;
    spiBuff[1] = rx_Channel;
    spiBuff[2] = 0x00;
    spiBuff[3] = 0x00;
    spiBuff[4] = 0x00;
    spiBuff[5] = 0x08;
    spiBuff[6] = 0x08;
    spiBuff[7] = 0x08;

    spiDriver->sendCmd(8, spiBuff);
}

/// @brief Prepare the radio to enter RX mode
void RadioAPI::radio_Start_RX()
{
    clearInterrupts();
    clearFifo(0x02);
    start_RX_Cmd();
}

/// @brief Read data stored in RX FIFO
/// @param msg Array stored with the RX FIFO data
void RadioAPI::read_RX_FIFO(const char *msg)
{
    get_RX_FIFO_Count();
    spiDriver->readData(0x77, pktSize, 0, (uint8_t *)msg);
    clearFifo(0x02);
    delayMicroseconds(500);
    clearInterrupts();
}

/// @brief Read interrupt data from radio
/// @param grp What group of interrupts to read from
/// @return interrupt group data
uint8_t RadioAPI::read_Interrupts(uint8_t grp)
{
    spiBuff[0] = 0x20;
    spiBuff[1] = 0x00;
    spiBuff[2] = 0x00;
    spiBuff[3] = 0x00;

    spiDriver->sendCmdGetResponse(4, spiBuff, 8, spiBuff);

    uint8_t index;
    switch (grp)
    {
    case 0:
        index = 2;
        break;
    case 1:
        index = 4;
        break;
    case 2:
        index = 6;
        break;

    default:
        return spiBuff[0];
        break;
    }

    return spiBuff[index];
}

/// @brief API command to configure certain properties of the radio
/// @param grp byte value specifing group
/// @param numProps number of properties to be configured
/// @param index where to start configuring properties
/// @param data Array of data to configure radio
void RadioAPI::set_Property(uint8_t grp, uint8_t numProps, uint8_t index, uint8_t *data)
{
    spiBuff[0] = 0x11;
    spiBuff[1] = grp;
    spiBuff[2] = numProps;
    spiBuff[3] = index;
    memcpy(&spiBuff[4], data, numProps);

    spiDriver->sendCmd(4 + numProps, spiBuff);
}

/// @brief Split or unsplit radio FIFO
/// @param enable_Bit 
void RadioAPI::radio_Enable_Split_FIFO(bool enable_Bit)
{
    if (enable_Bit)
    {
        // Set the max FILED_LENGTH_1 to 64 bytes and split the FIFO
        spiBuff[0] = (uint8_t)(64 >> 8);
        spiBuff[1] = (uint8_t)(64);
        set_Property(0x12, 0x02, 0x0D, spiBuff);
        spiBuff[0] = SPLIT_FIFO_MODE_ENABLE;
        set_Property(0x00, 0x01, 0x03, spiBuff);
    }
    else
    {
        // Set the max FILED_LENGTH_1 to 128 bytes and unsplit the FIFO
        spiBuff[0] = (uint8_t)(128 >> 8);
        spiBuff[1] = (uint8_t)(128);
        set_Property(0x12, 0x02, 0x0D, spiBuff);
        spiBuff[0] = SPLIT_FIFO_MODE_DISABLE;
        set_Property(0x00, 0x01, 0x03, spiBuff);
    }
}

/// @brief Read the configuration modes of radio FRRs
/// @param modes 
void RadioAPI::get_FRR_Mode(uint8_t* modes)
{
    memcpy(modes, &frr_modes[4], 4);
}

/// @brief Read the data from a specified FRR
/// @param index Which FRR to read
/// @return Data read from FRR
uint8_t RadioAPI::get_FRR_Data(uint8_t index)
{
    switch (index)
    {
    case 0:
        spiDriver->readData(0x50, 1, 0, spiBuff);
        break;
    case 1:
        spiDriver->readData(0x51, 1, 0, spiBuff);
        break;
    case 2:
        spiDriver->readData(0x53, 1, 0, spiBuff);
        break;
    case 3:
        spiDriver->readData(0x57, 1, 0, spiBuff);
        break;
    
    default:
        break;
    }
    return spiBuff[0];
}

// TODO: Heavy instruction and logic optimization
// TODO: Add Fixed-packet logic check
// bool RadioAPI::SearchFieldProperties(tSearchFieldParameters *fieldParams)
// {
//     uint16_t i = 0;
//     uint8_t cmdLen;
//     uint16_t leftPropertyInterval;
//     uint16_t rightPropertyInterval;
//     uint16_t positionInPro2CmdLine;
//     uint8_t elementPro2CmdLine;

//     uint16_t field_X_HighBytes[5] = {0x120D, 0x1211, 0x1215, 0x1219, 0x121D};
//     uint16_t field_X_LowBytes[5] = {0x120E, 0x1212, 0x1216, 0x121A, 0x121E};

//     for (int i = 0; i < 5; i++)
//         fieldParams->fieldLength[i] = 0;

//     fieldParams->fieldContainsLength = 0u;
//     fieldParams->fieldVaryInLength = 0u;
//     fieldParams->lengthStoredInFifo = 0u;
//     fieldParams->lengthFieldInByte = 0u;
//     fieldParams->lengthEndian = 0u;

//     while (config[i] != 0x00)
//     {
//         cmdLen = config[i++];

//         if (cmdLen > 16)
//         {
//             Serial.println("Command has more than 16 bytes of data");
//             return false;
//         }

//         for (uint8_t j = 0; j < cmdLen; j++)
//         {
//             spiBuff[j] = config[i++]; // Copy data AND move 'i' forward
//         }

//         if (spiBuff[1] == 0x12)
//         {
//             leftPropertyInterval = (spiBuff[1] << 8) | spiBuff[3];
//             rightPropertyInterval = (spiBuff[1] << 8) | spiBuff[2] + spiBuff[3] - 1;

//             // Determine which one of the fields [FIELD1,FIELD2, FIELD3, FIELD4] contains the length information
//             if (leftPropertyInterval <= property_PKT_LEN_FIELD_SOURCE && rightPropertyInterval >= property_PKT_LEN_FIELD_SOURCE)
//             {
//                 positionInPro2CmdLine = property_PKT_LEN_FIELD_SOURCE - leftPropertyInterval;
//                 elementPro2CmdLine = spiBuff[0x04 + (uint8_t)positionInPro2CmdLine];

//                 fieldParams->fieldContainsLength = elementPro2CmdLine & 0x7;
//                 // Serial.println("Field " + String(elementPro2CmdLine & 0x7) + " has length info");
//             }

//             // Determine which one of the fields [FIELD2,FIELD3, FIELD4, FIELD5] is variable part in the packet
//             if (leftPropertyInterval <= property_PKT_LEN && rightPropertyInterval >= property_PKT_LEN)
//             {
//                 positionInPro2CmdLine = property_PKT_LEN - leftPropertyInterval;
//                 elementPro2CmdLine = spiBuff[0x04 + (uint8_t)(positionInPro2CmdLine & 0xFF)];

//                 // Terminate the search if the DST_FIELD value is 0 or 1 (Cannot set field 1 as the varialbe field and 0 means there is not variable field)
//                 // if ((elementPro2CmdLine & 0x07 == 0x01) || elementPro2CmdLine & 0x07 == 0x00)
//                 //     return false;

//                 fieldParams->fieldVaryInLength = elementPro2CmdLine & 0x07;
//                 // Serial.println("Field that will vary: " + String(elementPro2CmdLine & 0x07));

//                 // Determine if length bytes(s) is stored in FiFO
//                 if ((elementPro2CmdLine & 0x08) == 0x00)
//                 {
//                     // Serial.println("Length bytes(s) not stored in FIFO");
//                     fieldParams->lengthStoredInFifo = 0x00;
//                 }
//                 else if ((elementPro2CmdLine & 0x08) == 0x08)
//                 {
//                     // Serial.println("Length bytes(s) stored in FIFO");
//                     fieldParams->lengthStoredInFifo = 0x01;
//                 }
//                 // Determine if size of field is 1 or 2 bytes
//                 if ((elementPro2CmdLine & 0x10) == 0x00)
//                 {
//                     // Serial.println("Field Length is 1 byte");
//                     fieldParams->lengthFieldInByte = 0x00;
//                 }
//                 else if ((elementPro2CmdLine & 0x10) == 0x10)
//                 {
//                     // Serial.println("Field Length is 2 byte");
//                     fieldParams->lengthFieldInByte = 0x02;
//                 }
//                 // Determine if Length field is little of big endian
//                 if ((elementPro2CmdLine & 0x20) == 0x00)
//                 {
//                     // Serial.println("Field Length is little-endian");
//                     fieldParams->lengthEndian = 0x00;
//                 }
//                 else if ((elementPro2CmdLine & 0x20) == 0x20)
//                 {
//                     // Serial.println("Field Length is big-endian");
//                     fieldParams->lengthEndian = 0x01;
//                 }
//             }
//         }
//         for (int i = 0; i < 5; i++)
//         {
//             if (leftPropertyInterval <= field_X_HighBytes[i] && field_X_HighBytes[i] <= rightPropertyInterval)
//             {
//                 positionInPro2CmdLine = field_X_HighBytes[i] - leftPropertyInterval;

//                 if (spiBuff[0x04 + (uint8_t)positionInPro2CmdLine] != 0)
//                     fieldParams->fieldLength[i] = ((uint16_t)spiBuff[0x04 + (uint8_t)positionInPro2CmdLine] << 8);
//                 else
//                     break;
//             }
//         }

//         for (int i = 0; i < 5; i++)
//         {
//             if (leftPropertyInterval <= field_X_LowBytes[i] && field_X_LowBytes[i] <= rightPropertyInterval)
//             {
//                 positionInPro2CmdLine = field_X_LowBytes[i] - leftPropertyInterval;

//                 if (spiBuff[0x04 + (uint8_t)positionInPro2CmdLine] != 0)
//                     fieldParams->fieldLength[i] = (uint16_t)spiBuff[0x04 + (uint8_t)positionInPro2CmdLine];
//                 else
//                     break;
//             }
//         }
//     }

//     uint8_t k;
//     for (k = 0; k < 5; k++)
//     {
//         if (fieldParams->fieldLength[k] == 0)
//             break;
//     }

//     // Set the remaining fields to zero
//     for (k++; k < 5; k++)
//     {
//         fieldParams->fieldLength[k] = 0u;
//     }

//     if ((fieldParams->fieldLength[0u] == 0u) &&
//         (fieldParams->fieldLength[1u] == 0u) &&
//         (fieldParams->fieldLength[2u] == 0u) &&
//         (fieldParams->fieldLength[3u] == 0u) &&
//         (fieldParams->fieldLength[4u] == 0u))
//     {
//         Serial.println("All fields are 0");
//         return false;
//     }
//     return true;
// }
