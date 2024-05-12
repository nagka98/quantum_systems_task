/******************************************************************************
This is the embedded C task. The program simulates a battery

*******************************************************************************/

#include<stdio.h>
#include<string.h>
#include<stdbool.h>
#include "battery_simulator.h"

#include "kalman_filter.h"


/******************************************************************************/
/*
DATASTRUCTURE
typedef struct {
    float timestamp;    in Seconds
    int voltage;        in mV
    int current;        in mA
    int temperature;    in 0.1C
} measured_values;
*/

typedef struct {
    float count;
}kal_filter_count;

kal_filter_count kal_curr = {0};
kal_filter_count kal_volt = {0};

/******************************************************************************
DEFINES
*******************************************************************************/
#define ALERT(msg, ...) printf("WARNING_ALERT :" msg , __VA_ARGS__)

#define CURRENT_MODE 0
#define VOLTAGE_MODE 1

/******************************************************************************
FUNCTION PROTOTYPES
*******************************************************************************/

void check_temperature(measured_values* pData);
int kalman_filter(measured_values* pData, bool mode);

/******************************************************************************
GLOBAL VARIABLES AND POINTERS
*******************************************************************************/
/*create instance to acess csv file*/
FILE *csv_file;

/******************************************************************************
FUNCTION BODY
*******************************************************************************/
int main(){
    int est_current, est_voltage = 0;

    /* open csv file */
    csv_file = fopen("battery_log.csv", "w");
    measured_values data;

    /* create cell header for battery properties */
    fprintf (csv_file, "Voltage, Current \n");

    /*Battery Simulator*/
    for (int i = 0; i < 4300; i++) 
    {
        //Task0 Get it running 	  
		//Task1 check_temperature() Check for overtemperature
		//Task2 filter() filter the voltage and the current
		//Task3 calculate_SOC() calculate the SoC based on the state of the battery

        /* receive model data from battery simulator */
        measure(&data);

        // printf ("Timestamp: %f, Voltage: %d, Current: %d, Temperature: %d \n",
		// 	  data.timestamp, data.voltage, data.current,
		// 	  data.temperature);

        check_temperature(&data);

        int temp = kalman_filter(&data, CURRENT_MODE);
        if(temp != 0xffff)
        {
            est_current = temp;
            // printf("estimated current : %d\n", est_current);
        }
        temp = kalman_filter(&data, VOLTAGE_MODE);
        if(temp != 0xffff)
        {
            est_voltage = temp;
            // printf("estimated voltage : %d\n", est_voltage);
        }
        /* print battery characterstics in csv file */      
        fprintf (csv_file, "%d, %d \n",
			  est_voltage, est_current);
    }

    printf("End of simulation \n");
    fclose(csv_file);
    return 0;
}

/**
 * TASK 1
 * 
 * As per section 1.3@datasheet: 
 * The battery shall be charged within 10℃~45℃ range in the Product Specification
 *  
 * As per section 2.2@datasheet:
 * The battery discharge temperature is -20~60℃,10~45℃ environment suggested when discharge with large 
 * current,small current discharge suggested under ＜10℃ or ＞45℃, Discharged under too low or too high temperature 
 * could lead to battery failure or other conditions
 * 
 * NOTE: As per specification 13, standard discharge current is 2.34A
 * lets consider,
 * 
 * @small_current:          (0A <= current < 2.34A), temperature limits:{-20~60℃} 
 * @large_current:          (current >= 2.34A), temperature limits:{10~45℃} 
 * (sink)@reverse_current:  (current < 0A), temperature limits:{10~45℃} 
 *  
*/
void check_temperature(measured_values* pData)
{
    /* check for small discharge current */
    if((pData->current < 2340)&&(pData->current >= 0))
    {   
        if((pData->temperature > 600)||(pData->temperature < -200))
        {
            ALERT("TEMPERATURE out of bounds(-20°C <-> 60°C), current(A) : %0.001f, temperature(°c) = %0.1f \n", 
            (float)(pData->current * 0.001f), (float)(pData->temperature * 0.1f));
        }
    }
    else if((pData->current >= 2340)||(pData->current < 0))/* check for large or sinking currents */
    {
        if((pData->temperature > 450)||(pData->temperature < 100))
        {
            ALERT("TEMPERATURE out of bounds(10°C <-> 45°C), current(A) : %0.001f, temperature(°c) = %0.1f \n", 
            (float)(pData->current * 0.001f), (float)(pData->temperature * 0.1f));
        }

    }
}

/** TASK 2
 * 
 * Third party kalman filter modified with default values from @https://gist.github.com/jannson/9951716
 * 
 * Function kalman_filter
 * @param: 
 * pdata[measured_values*]: pointer towards battery data
 * mode[bool]: current 0, voltage 1
 * Default calculation based on past 5 values
 * @return[int]: kalman filter estimation output
 * 
*/

int kalman_filter(measured_values* pData, bool mode)
{
    kal_filter_count* temp_struct;

    if(mode == CURRENT_MODE)
    {
        temp_struct = &kal_curr;
    }
    else
    {
        temp_struct = &kal_volt;
    }

    /* check if we reached limit */
    if(temp_struct->count == 0)
    {
        kalman_filter_reset(mode);
    }

    temp_struct->count++;

    int value = (mode == 0)? pData->current : pData->voltage;
    int temp_pred = kal_filter_predict(mode, value);

    if(temp_struct->count == 5)
    {
        temp_struct->count = 0;
        return temp_pred;
    }
    return 0xffff;
}