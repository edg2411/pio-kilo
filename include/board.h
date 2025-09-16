// Board configuration header file
// Centralizes pin configurations and board-specific settings

#ifndef BOARD_H
#define BOARD_H

// Ethernet SPI pins for W5500
#define ETHERNET_SCK_PIN   25
#define ETHERNET_MISO_PIN  23
#define ETHERNET_MOSI_PIN  27
#define ETHERNET_CS_PIN    18

// Ethernet PHY configuration for W5500
#define ETHERNET_PHY_TYPE       ETH_PHY_W5500
#define ETHERNET_PHY_ADDR       1
#define ETHERNET_PHY_IRQ        -1
#define ETHERNET_PHY_RST        -1

// LTE Serial configuration
#define LTE_SERIAL_NUM      2
#define LTE_SERIAL_BAUD     115200
#define LTE_RX_PIN          16
#define LTE_TX_PIN          17

// LTE Modem type
#define LTE_MODEM_TYPE      PPP_MODEM_SIM7600

// Test board pins
#define LED_PIN         14
#define RELAY_PIN       15
#define BUTTON_PIN      12
#define BUZZER_PIN      21

#endif // BOARD_H