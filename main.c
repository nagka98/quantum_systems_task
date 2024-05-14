/******************************************************************************
This is the embedded C task. The program simulates a battery

*******************************************************************************/

#include<stdio.h>
#include<string.h>
#include<stdbool.h>
#include<math.h>
#include "battery_simulator.h"
#include "soc_table.h"
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
    int kal_voltage;       
    int kal_current;   
    int initial_soc;
    bool initial_soc_flag; 
    int curr_timesetamp;
    int last_timestamp;
} calc_values;

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

#define TOTAL_COLUMB 84600.0f

/******************************************************************************
FUNCTION PROTOTYPES
*******************************************************************************/

void check_temperature(measured_values* pData);
int kalman_filter(measured_values* pData, bool mode);
int calculate_SOC(calc_values* pKalData);
int opencircuit_soc(calc_values* pKalData);
int columbcount_soc(calc_values* pKalData);

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

    calc_values kalvalue = {0};

    /* open csv file */
    csv_file = fopen("battery_log.csv", "w");
    measured_values data;

    /* create cell header for battery properties */
    fprintf (csv_file, "SoC \n");

    int current_soc, kalman_sweep_cplt = 0;

    /*Battery Simulator*/
    for (int i = 0; i < 4300; i++) 
    {
        //Task0 Get it running 	  
		//Task1 check_temperature() Check for overtemperature
		//Task2 filter() filter the voltage and the current
		//Task3 calculate_SOC() calculate the SoC based on the state of the battery

        /* receive model data from battery simulator */
        measure(&data);

        kalvalue.last_timestamp = kalvalue.curr_timesetamp;
        kalvalue.curr_timesetamp = data.timestamp;

        // printf ("Timestamp: %f, Voltage: %d, Current: %d, Temperature: %d \n",
		// 	  data.timestamp, data.voltage, data.current,
		// 	  data.temperature);

        check_temperature(&data);

        int temp = kalman_filter(&data, CURRENT_MODE);
        if(temp != 0xffff)
        {
            kalvalue.kal_current = temp;
            kalman_sweep_cplt++;
        }
        temp = kalman_filter(&data, VOLTAGE_MODE);
        if(temp != 0xffff)
        {
            kalvalue.kal_voltage = temp;
            kalman_sweep_cplt++;
        }

        if(kalman_sweep_cplt >= 2)
        {
            current_soc = calculate_SOC(&kalvalue);
            kalman_sweep_cplt = 0;
            // printf("current_soc = %d \n",current_soc);
            fprintf(csv_file, "%0.2f \n",current_soc*0.01f);
        }
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
 * 
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

/** TASK 3
 * 
 * Function calculate_SOC()
 * 
 * @implementation:
 * follows two stages
 * 1) get current estimate by open circuit voltage analysis
 * 2) get SoC during battery discharge by columb counting 
 * 
 * @limitations: 
 * ->this method does not consider aging factor of battery
 * ->not every battery battery is ideal. tolerances should be adjusted
 * ->completely depends on initial consumption calculation
 * ->battery lifetime can also be affected by operating temperature(not considered)
 * 
 * filtered outputs provide better estimation of soc
 * 
 * @param: 
 * method[bool]: open_circuit (0), columb_counting (1)
 * @return[int]: State of Charge percentage in integer (*10)
 * 
*/

int calculate_SOC(calc_values* pKalData)
{
    static bool initial_sweep = false;

    if(!initial_sweep)
    {
        initial_sweep = true;
        pKalData->initial_soc = opencircuit_soc(pKalData);
        pKalData->initial_soc_flag = true;
        printf("initial sweep : %d \n", pKalData->initial_soc);
    }

    int soc_result = columbcount_soc(pKalData);
    return soc_result;
}

/**
 * Function opencircuit_soc()
 * 
 * 7S2P configuration
 * voltage of each cell : (total_voltage)/7
*/
int opencircuit_soc(calc_values* pKalData)
{
    float cell_voltage = (float)pKalData->kal_voltage/7.0f;

    float *pSoc_table = GetSOCTable();
    float temp_v1 = 0;

    for(int i = 0; i < GRP9667124_SOCTABLE_LEN; i++)
    {
        temp_v1 = cell_voltage - pSoc_table[i];
        if(temp_v1 < 0.0f)
        {
            return i;
        }
    }
    return 0xffff;
}

/**
 * function columbcount_soc()
 * 
 * ref from datasheet Specification (nominal capacity : 11750mAh,26.95V, 7s1p)
 * total columb capacity of 7s2p battery as per seconds : 11.750 * 2 * 60 * 60 = 84600.000f columbs @TOTAL_COLUMB
 * removing time integrated current from total columb capacity gives remaining capacity
*/
int columbcount_soc(calc_values* pKalData)
{   
    static float columb_consumed = 0.0f;
    float batt_capcity_left = TOTAL_COLUMB - ((100 - pKalData->initial_soc)*0.01*TOTAL_COLUMB);
    columb_consumed += (float)(pKalData->curr_timesetamp - pKalData->last_timestamp) * (float)(pKalData->kal_current * 0.001f);
    float soc_left = (batt_capcity_left - columb_consumed) * (100.0f/TOTAL_COLUMB);
    soc_left = (soc_left > 0.0f)? soc_left : 0.0f;
    printf("soc left : %0.2f\n",soc_left);
    return (int)(soc_left * 100);
}
