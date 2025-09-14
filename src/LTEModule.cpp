#include "LTEModule.h"
#include "board.h"

LTEModule::LTEModule(HardwareSerial* serial, int rst, int tx, int rx, int rts, int cts) : serial(serial), connected(false) {
    // Configure PPP
    PPP.setApn(apn.c_str());
    PPP.setPin("0000");  // Default PIN
    if (rst != -1) PPP.setResetPin(rst, false, 200);  // Active HIGH, 200ms delay
    if (tx != -1 && rx != -1) {
        PPP.setPins(tx, rx, rts, cts, (rts != -1 && cts != -1) ? ESP_MODEM_FLOW_CONTROL_HW : ESP_MODEM_FLOW_CONTROL_NONE);
    }
    // For Quectel EC25, use PPP_MODEM_SIM7600 or similar
    PPP.begin(LTE_MODEM_TYPE);
}

void LTEModule::setAPN(const String& apn, const String& user, const String& pass) {
    this->apn = apn;
    this->user = user;
    this->pass = pass;
    PPP.setApn(apn.c_str());
    PPP.setPin(pass.c_str());  // Use pass as PIN if provided
}

bool LTEModule::connect() {
    if (connected) return true;
    if (apn.isEmpty()) return false;

    // Wait for network attachment (reduced blocking)
    bool attached = PPP.attached();
    if (!attached) {
        for (int i = 0; i < 10 && !attached; i++) {  // 10 * 100ms = 1s max
            delay(100);
            attached = PPP.attached();
        }
    }

    if (attached) {
        // Switch to data mode
        PPP.mode(ESP_MODEM_MODE_CMUX);
        if (PPP.waitStatusBits(ESP_NETIF_CONNECTED_BIT, 1000)) {
            connected = PPP.connected();
            return connected;
        }
    }
    return false;
}

void LTEModule::disconnect() {
    connected = false;
}

bool LTEModule::isConnected() {
    return connected;
}

IPAddress LTEModule::getIP() {
    return IPAddress(0,0,0,0); // Stub
}