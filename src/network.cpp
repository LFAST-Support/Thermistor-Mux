
/*******************************************************************************
Copyright 2021
Steward Observatory Engineering & Technical Services, University of Arizona
This program is free software: you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or any later version.
This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.
You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*******************************************************************************/

/**
 * @file network.cpp
 * @author Rory Scobie (scobier@arizona.edu)
 * @brief Implements networking specific functions, to handle Ethernet, MQTT,
 * Sparkplug, and NTP functionality.
 * @version (see VCM_VERSION in vcm_global.h)
 * @date 2021-03-10
 *
 * @copyright Copyright (c) 2021
 */

/*
Will try and implement Rory's Network configuration files for the teensy, it's not 
fully present yet because i'm tyring to go througha nd understand what it's doing 
as I paste it in this file.  
*/


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


void printData(char* name, float data) { //Convert digital data, print/ send through network payload. 
    
    Serial.print(name);
    Serial.println(data, 4);
}