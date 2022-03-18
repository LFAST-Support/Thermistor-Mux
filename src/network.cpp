#include "network.h"
//#include <Arduino.h>
#include <Ethernet.h>

#define csPin 0
#define dev_MAC {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}
#define dev_IP {000, 000, 0, 000}


bool initNetwork() {

    IPAddress ip = dev_IP;
    byte mac[6] = dev_MAC;

    Ethernet.init(csPin);
    Ethernet.begin(mac, ip);


    return true;
}
