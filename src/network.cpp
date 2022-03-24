#include "network.h"
//#include <Arduino.h>
#include <NativeEthernet.h>
//#include <PubSubClient.h>
//#include <NTPClient_Generic.h>


// Common network configuration values
#define GATEWAY 192, 168, 1, 1
#define SUBNET 255, 255, 255, 0
#define DNS 192, 168, 1, 1

#define NUM_BROKERS  1

//#if defined( PRODUCTION )
// MQTT broker definitions for production
#define MQTT_BROKER 192, 168, 1, 51
#define MQTT_BROKER1_PORT 1883

// NTP server address
#define NTP_IP  {192, 168, 1, 10}

#define BASE_MAC #define {0x0, 0x0, 0x0, 0x0, 0x0, 0x0}

#define BASE_IP {192, 168, 1, 160}

// MQTT variables
static EthernetClient enet[ NUM_BROKERS ];
//static PubSubClient broker[ NUM_BROKERS ];

#define csPin 0

bool initNetwork() {


    Ethernet.init(csPin);

    return true;
}


void printData(float data) { //Convert digital data, print/ send through network payload. 
      Serial.println(data, 4);
}