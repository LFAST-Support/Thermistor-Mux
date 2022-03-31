
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
 * @file vcm_network.h
 * @author Rory Scobie (scobier@arizona.edu)
 * @brief Networking specific definitions and function prototypes.  For NTP,
 * MQTT, Sparkplug, and Ethernet.
 * @version (see VCM_VERSION in vcm_global.h)
 * @date 2021-03-10
 *
 * @copyright Copyright (c) 2021
 */


#include <avr/io.h>


#ifndef NETWORK_H
#define NETWORK_H

bool initNetwork();
void printData(char* name, float);


#endif
