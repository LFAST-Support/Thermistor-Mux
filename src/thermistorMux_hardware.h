

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