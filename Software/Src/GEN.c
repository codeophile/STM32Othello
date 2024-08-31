/********************************************************************************
GEN.c - GENERAL USE STUFF
********************************************************************************/

#include "APP.h"
#include "main.h"

#define  ARM_CM_DEMCR      (*(uint32_t *)0xE000EDFC)
#define  ARM_CM_DWT_CTRL   (*(uint32_t *)0xE0001000)
#define  ARM_CM_DWT_CYCCNT (*(uint32_t *)0xE0001004)

//------------------------------------------------------------------------------
uint GEN_GetDeltaCycles(void) {

       //(See comments in INIT below)
       //Returns number of CPU cycles that has elapsed since the last time this was called
       //I tested this using the SysTick_Handler() set to callback every 1mS. DeltaCycles reads 119,984 (1/1000 of the CPU freq.
       //Counter rolls over every 0x100000000/sysclk seconds = 35.8 secs but calc is still valid on roll over

       static uint saved;
       uint current = ARM_CM_DWT_CYCCNT;
       uint elapsed = current - saved;

       //For next pass
       saved = current;
       return elapsed;
}


//------------------------------------------------------------------------------
void GEN_GetDeltaCycles_INIT(void) {

       //This INIT is not necessary on the FRDM-K64F but is necessary on the STM32
       //Source:https://www.embedded-computing.com/articles/measuring-code-execution-time-on-arm-cortex-m-mcus
       //Also see https://stackoverflow.com/questions/36378280/stm32-how-to-enable-dwt-cycle-counter

       if (ARM_CM_DWT_CTRL != 0) {  //If DWT is available
              ARM_CM_DEMCR |= 1 << 24; //Set bit 24
              ARM_CM_DWT_CYCCNT = 0;
              ARM_CM_DWT_CTRL |= 1 << 0; //Set bit 0
       }
}
