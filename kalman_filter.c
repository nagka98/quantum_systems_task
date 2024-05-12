
/** A simple kalman filter example by Adrian Boeing 
 www.adrianboeing.com 
 */ 

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>

//initial values for the kalman filter
//the noise in the system
float Q = 0.01;
float R = 0.01;
float K;
float P;
float P_temp;
float x_temp_est;
float x_est;

typedef struct 
{
    float x_est_last;
    float P_last;
}kal_values;

kal_values g_kal_current = {0};
kal_values g_kal_voltage = {0};

double frand() {
    return 2*((rand()/(double)RAND_MAX) - 0.5);
}

void kalman_filter_reset(bool mode) {
    
    srand(0);
    
    //initialize with a measurement 
    //initial values for the kalman filter
    if(mode == 0)
    {
        memset(&g_kal_current, 0, sizeof(g_kal_current));
    }
    else
    {
        memset(&g_kal_voltage, 0, sizeof(g_kal_voltage));
    }
    return;
}

int kal_filter_predict(bool mode, int new_value)
{
    kal_values* temp_struct;
    if(mode == 0)
    {
        temp_struct = &g_kal_current;
    }
    else
    {
        temp_struct = &g_kal_voltage;
    }

    //do a prediction
    x_temp_est = temp_struct->x_est_last;
    P_temp = temp_struct->P_last + Q;
    //calculate the Kalman gain
    K = P_temp * (1.0/(P_temp + R));
    //correct
    x_est = x_temp_est + K * ((float)new_value - x_temp_est); 
    P = (1- K) * P_temp;

    temp_struct->x_est_last = x_est;
    temp_struct->P_last = P;


    return (int)x_est;
}