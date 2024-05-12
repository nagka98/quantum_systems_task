/******************************************************************************/
/* DO NOT EDIT THIS FILE */
/******************************************************************************/
#ifndef BATTERY_SIMULATOR_H
#define BATTERY_SIMULATOR_H

#include<stdbool.h>

/******************************************************************************/
/* DO NOT EDIT THIS FILE */
/******************************************************************************/

typedef struct {
    float timestamp;    //in Seconds
    int voltage;        //in mV
    int current;        //in mA
    int temperature;    //in 0.1C
} measured_values;

/*** Public functions ***/
void measure();



#endif /* BATTERY_SIMULATOR_H */