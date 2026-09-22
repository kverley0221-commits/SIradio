#pragma once

// enum class FRR_MODES
// {
//     DISABLED,
//     INT_STATUS,
//     INT_PEND,
//     INT_PH_STATUS,
//     INT_PH_PEND,
//     INT_MODEM_STATUS,
//     INT_MODEM_PEND,
//     INT_CHIP_STATUS,
//     INT_CHIP_PEND,
//     CURRENT_STATE,
//     LATCHED_RSSI
// };

struct pendingType
{
    uint8_t INT_PEND; // LSB
    uint8_t PH_PEND;
    uint8_t MODEM_PEND;
    uint8_t CHIP_PEND; // MSB
};

union pending_Interrupts
{
    pendingType type;
    uint32_t interrupt;
};

union frr_registers
{
    uint8_t registers[4];
    uint32_t frr_values;
};

#define RX_FIFO_ALMOST_FULL_PEND 0x01
#define TX_FIFO_ALMOST_EMPTY_PEND 0x02
#define ALT_CRC_ERROR_PEND 0x04
#define CRC_ERROR_PEND 0x08
#define PACKET_RX_PEND 0x10
#define PACKET_SENT_PEND 0x20
#define FILTER_MISS_PEND 0x40
#define FILTER_MATCH_PEND 0x80

#define SI446X_CMD_FIFO_INFO_ARG_FIFO_TX_BIT 0x01
#define SI446X_CMD_FIFO_INFO_ARG_FIFO_RX_BIT 0x02

#define SPLIT_FIFO_MODE_ENABLE 0X00
#define SPLIT_FIFO_MODE_DISABLE 0X10

#define TX_THRESHOLD_INDEX 0x0B
#define TX_THRESHOLD_INDEX 0x0C

#define FIFO_MODE_INDEX 0x03
