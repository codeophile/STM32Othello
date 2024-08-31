/********************************************************************************
AIN.c
********************************************************************************/

//MX tool setup:
//Selected PB1 as ADC1_IN16
//In ADC1 Mode section checked: IN16 Single-ended
//Left all other settings as default
//Apparently, the code below works without specifying a channel
//because only one channel was configured.
//With 80 MHz clock, the function below took 12uS.

#include "APP.h"
#include "main.h"

extern ADC_HandleTypeDef hadc1;

float AIN_GetBatVolts(){

	//Takes 12uS
	//Get ADC1 value.
	HAL_ADC_Start(&hadc1);
	HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
	float raw = (float)HAL_ADC_GetValue(&hadc1);
	float volts = raw/4096.0f *3.3f;
	return volts;

}
