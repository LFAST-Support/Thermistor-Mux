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
 * @file thermistorMux_network.h
 * @author Rory Scobie (scobier@arizona.edu)
 * @brief Networking specific definitions and function prototypes.  For NTP,
 * MQTT, Sparkplug, and Ethernet. 
 * Originally created for VCM module, modified by Nestor, for thermistor Mux use
 * @version (see THERMISTOR_MUX_VERSION in thermistorMux_global.h)
 * @date 2022-04-19
 *
 * @copyright Copyright (c) 2021
 */

#ifndef THERMISTOR_MUX_H
#define THERMISTOR_MUX_H

bool cal_thermistor(float set_temp, int tempNum);
bool clear_cal_data();

#endif


