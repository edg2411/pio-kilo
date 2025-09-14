// Board configuration header file
// Centralizes pin configurations and board-specific settings

#ifndef BOARD_H
#define BOARD_H

// Ethernet SPI pins for W5500
#define ETHERNET_SCK_PIN   14
#define ETHERNET_MISO_PIN  12
#define ETHERNET_MOSI_PIN  13
#define ETHERNET_CS_PIN    15

// Ethernet PHY configuration for W5500
#define ETHERNET_PHY_TYPE       ETH_PHY_W5500
#define ETHERNET_PHY_ADDR       1
#define ETHERNET_PHY_IRQ        4
#define ETHERNET_PHY_RST        5

// LTE Serial configuration
#define LTE_SERIAL_NUM      2
#define LTE_SERIAL_BAUD     115200
#define LTE_RX_PIN          16
#define LTE_TX_PIN          17

// LTE Modem type
#define LTE_MODEM_TYPE      PPP_MODEM_SIM7600

#endif // BOARD_H