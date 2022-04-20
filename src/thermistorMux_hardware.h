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
 * @file tehermistorMux_hardware.h
 * @author Nestor Garcia (Nestor212@email.arizona.edu)
 * @brief Function prototype(s) for device ID initializtion.
 * @version INW
 * @date 2022-04-19
 *
 * @copyright Copyright (c) 2022
 */


#ifndef THERMISTORMUX_HARDWARE_H
#define THERMISTORMUX_HARDWARE_H

#define ID_PIN_0 39
#define ID_PIN_1 38
#define ID_PIN_2 35
#define ID_PIN_3 34
#define ID_PIN_4 33 
static int hardware_id = -1;

bool hardwareID_init();
int get_hardware_id();

#endif