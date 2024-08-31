/********************************************************************************
APP.c - My "main"
********************************************************************************/

#include "APP.h"
#include "main.h"
#include "DIN.h"
#include <stdlib.h> //for seeding for random moves


bool digUpdate;
uint digCount;
bool WSCMDUpdate;
uint WSCMDCount;
bool mainUpdate;
uint mainCount;

uint elapsedTics;
uint maxUpdateTics;

//------------------------------------------------------------------------------
void HAL_IncTick(void){

	//STANDARD CODE
	//This function overrides the weak function called by SysTick_Handler(void) (stm32l4xx_it.c)
	//Keep the one line of code that's in the weak function. uwTick is used by HAL_GetTick()
	//which is in turn used by HAL_Delay() and HAL function timeouts
	uwTick += (uint32_t)uwTickFreq;


	//APP SPECIFIC CODE **************************
	//Note I don't do anything that does a lot of CPU here to keep ticks accurate
	if(++digCount==5){
		digCount=0;
		digUpdate=true;
	}
	if(++WSCMDCount==10){
		WSCMDCount=0;
		WSCMDUpdate=true;
	}
	//Want 5/sec. Used 199 which is prime number to minimize update overlap (not that it really matters)
	if(++mainCount==199){
		mainCount=0;
		mainUpdate=true;
	}


}

//------------------------------------------------------------------------------
void APP_Main(){

	void MainUpdate();
	void MainInit();
	float AIN_GetBatVolts();
	void WS64_CmdListUpdate();

	GEN_GetDeltaCycles_INIT();
	GEN_GetDeltaCycles();

	//Seed random number generator using battery voltage back calculated to raw counts
	float volts=AIN_GetBatVolts();
	uint raw = (uint)(4096.0*volts/3.3);
	srand(raw);

	//Main initialize
    MainInit();

	while(1){

		elapsedTics=GEN_GetDeltaCycles();

		if(digUpdate){
			digUpdate=false;
			DIN_Update();
		}

		if(WSCMDUpdate){
			WSCMDUpdate=false;
			WS64_CmdListUpdate();
		}

		if(mainUpdate){
			mainUpdate=false;
			MainUpdate();
		}

		//Store max update duration for purpose of code analysis.
		//Clock is 80,000,000
		//Max is around 370,000 which equates to 4.6 mS which is mostly due to the time needed to load the
		//SPI register with the board colors and wait for SPI to complete (I use the blocking variant)
		elapsedTics=GEN_GetDeltaCycles();
		if(maxUpdateTics<elapsedTics)maxUpdateTics=elapsedTics;

	}//while
}








