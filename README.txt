==============================
QS Programming task
==============================

In this programming task, a battery with the states idle, discharge and empty is simulated.
This means the measure() function provides simulated data based on the internal state of the battery.
At the beginning the mosfets of the battery are open, so the battery is in the so called 'Open Circuit' mode.
Therefore there is no current flow.
After activating the mosfets the battery is in discharge mode and will provide data which represents a flight with our UAV.
After some time the battery is empty and is disconnected from the load. 

The datastructure is

typedef struct {
    float timestamp;    in Seconds
    int voltage;        in mV
    int current;        in mA
    int temperature;    in 0.1C
} measured_values;

Your task is to process the incoming data in different ways inside main.c
There is a main loop ready to run in main.c. 
When you start the program simulated values are printed.

The simulated battery is in a 7s2p configuration.
You got the datasheet for the battery with the instructions.

Task 0 - Get Things up and running
====================================================
As we already pointed out, you are free to choose a complier and IDE. What fits for you fits for us ;-)
To make things easier we recommend the usage of https://www.onlinegdb.com

Task 1 - Check Temperature
====================================================
Implement a temperature check based on the datasheet for the cells.
Print a warning when the battery gets too hot.
Please explain in a comment how you choose your thresholds. 

Task 2 - Filter Voltage and Current
====================================================
Implement a filter on the raw voltage in mV to filter at least the last 5 values. Print the result
Implement a filter on the raw current in mA to filter at least the last 5 values. Print the result

Task 3 - SoC extimation and remaining capacity
====================================================
Implement a function which calculates the SoC during the simulation. 
Please use two different SoC calculation methods. Recommended are a OpenCircuit estimation (see soc_table) and Coulumb Counting. 
The soc_table maps the voltage from one cell to a SoC in percentage.
Please explain in a comment your implementations and limits of the soc estimation methodes.
Please comment if it is better to use the filtered or the raw current value.
Print the calculated SOC and the remaining capacity of the battery.
Store the calculated SOC in a CSV file. (Each value in a new line)