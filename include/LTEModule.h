#ifndef LTE_MODULE_H
#define LTE_MODULE_H

#include <HardwareSerial.h>
#include <PPP.h>
#include <Arduino.h>

class LTEModule {
private:
    HardwareSerial* serial;
    String apn;
    String user;
    String pass;
    bool connected;

public:
    LTEModule(HardwareSerial* serial, int rst = -1, int tx = -1, int rx = -1, int rts = -1, int cts = -1);
    void setAPN(const String& apn, const String& user = "", const String& pass = "");
    bool connect();
    void disconnect();
    bool isConnected();
    IPAddress getIP();
};

#endif // LTE_MODULE_H