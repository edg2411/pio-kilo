#include "EthernetModule.h"
#include "board.h"

EthernetModule::EthernetModule(int sck, int miso, int mosi, int cs, int addr, int irq, int rst) : connected(false), sck(sck), miso(miso), mosi(mosi), cs(cs), addr(addr), irq(irq), rst(rst) {
    // Default MAC, can be set later
    mac[0] = 0xDE; mac[1] = 0xAD; mac[2] = 0xBE; mac[3] = 0xEF; mac[4] = 0xFE; mac[5] = 0xED;
    ip = IPAddress(192, 168, 1, 100);
    gateway = IPAddress(192, 168, 1, 1);
    subnet = IPAddress(255, 255, 255, 0);
}

void EthernetModule::setConfig(byte mac[6], IPAddress ip, IPAddress gateway, IPAddress subnet) {
    memcpy(this->mac, mac, 6);
    this->ip = ip;
    this->gateway = gateway;
    this->subnet = subnet;
}

bool EthernetModule::connect() {
    if (connected) return true;
    SPI.begin(sck, miso, mosi, cs);
    ETH.begin(ETHERNET_PHY_TYPE, addr, cs, irq, rst, SPI);
    ETH.config(ip, gateway, subnet);
    // No delay, check linkUp in isConnected
    return false;
}

void EthernetModule::disconnect() {
    // Ethernet doesn't have disconnect, but we can set connected false
    connected = false;
}

bool EthernetModule::isConnected() {
    connected = ETH.linkUp();
    return connected;
}

IPAddress EthernetModule::getIP() {
    return ETH.localIP();
}

String EthernetModule::getMAC() {
    byte currentMac[6];
    Network.macAddress(currentMac);
    // Update stored mac
    memcpy(this->mac, currentMac, 6);
    // Format as string
    char macStr[18];
    sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", currentMac[0], currentMac[1], currentMac[2], currentMac[3], currentMac[4], currentMac[5]);
    return String(macStr);
}