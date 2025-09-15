#ifndef ETHERNET_MODULE_H
#define ETHERNET_MODULE_H

#include <ETH.h>
#include <SPI.h>
#include "board.h"

class EthernetModule {
private:
    byte mac[6];
    IPAddress ip;
    IPAddress gateway;
    IPAddress subnet;
    IPAddress dns1;
    IPAddress dns2;
    bool connected;
    bool staticIPEnabled;
    int sck, miso, mosi, cs, addr, irq, rst;

public:
    EthernetModule(int sck = ETHERNET_SCK_PIN, int miso = ETHERNET_MISO_PIN, int mosi = ETHERNET_MOSI_PIN, int cs = ETHERNET_CS_PIN, int addr = ETHERNET_PHY_ADDR, int irq = ETHERNET_PHY_IRQ, int rst = ETHERNET_PHY_RST);
    void setConfig(byte mac[6], IPAddress ip, IPAddress gateway, IPAddress subnet);
    void setStaticIP(IPAddress ip, IPAddress gateway, IPAddress subnet, IPAddress dns1, IPAddress dns2);
    void enableStaticIP(bool enabled);
    bool connect();
    void disconnect();
    bool isConnected();
    IPAddress getIP();
    String getMAC();
};

#endif // ETHERNET_MODULE_H