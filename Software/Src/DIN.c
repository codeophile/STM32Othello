/********************************************************************************
DIN.c
********************************************************************************/
#include "APP.h"
#include "main.h"

	//THIS VAR IS ALL THAT CONSUMER NEEDS
	//Button state changes are latched here until consumer resets
	uint DIN_buttonLatch=0;

	//---------------------------------------
	//BELOW ONLY USED INTERNALLY
	//---------------------------------------

	//Initialize debounce to unpressed values (else get one false keypress at startup)
	uint debounced=0;
	uint btnsPrev=0;
	
	//A 2 bit "vertical" counter
	uint bits0=0;
	uint bits1=0;
	
void UpdateButtonLatch(){

	//Called by DIN_Update() below
	//Stores values in DIN_buttonLatch which must be reset by consumer code.
	//Reflects 8 bits PA0-7
	//Lower 8 bits go true when switch changes from false true
	//Upper 8 bits go true when switch changes from true to false;

	uint btnsNow = debounced&0xFF;
	//Set true any bits that just changed
	uint btnsJustWentTrue = (~btnsPrev) & btnsNow; //were 0 now 1
	uint btnsJustWentFalse= btnsPrev & (~btnsNow); //were 1 now 0
	//Store for next pass
	btnsPrev=btnsNow;
	uint justWent = btnsJustWentTrue | (btnsJustWentFalse<<8);
	//if(justWent!=0)
		DIN_buttonLatch |= justWent;
}

void DIN_Update(){

	//Get raw data packed into a single 32 bit uint
	//Bits are inverted because switch inputs low true (normally high)
	uint raw = ~GPIOA->IDR;

	//DEBOUNCED VALUE CHANGES TO RAW VALUE ONCE RAW VALUE HAS BEEN DIFFERENT FOR 4 *CONSECUTIVE* UPDATES
	
	//Uses a 2 bit "vertical" counter
	//For example 4 vertical counters from left to right are 3,2,1,0
	//bits1=1100	
	//bits0=1010

	//For understanding the below, it's helpful to know that a 2-input XOR gate acts like a 
	//conditional inverter:
	//One input is the one that will be inverted or not, the other is the control.
	//Write as output = input ^ control;
	//if control=1 then output = ~input else output = input

	//Store changed bits.
	uint changed=raw^debounced;
	
	//If any bit is 0 (unchanged), clear it's counter
	bits0 &= changed;
	bits1 &= changed;
	
	//If bit has changed and it's counter is binary 11 that means this is the 4th 
	//consecutive update in which the value has changed.
	//Use the conditional inverter (described on top) to invert the debounced bi
	debounced ^= (changed & bits0 & bits1);
	
	//Finally, for bits that are changed increment it's counters by inverting them.
	//If bit changed and bits0=1, invert bits1
	bits1 ^= (changed & bits0);
	//If bits changed invert bits0
	bits0 ^= changed;
	
	UpdateButtonLatch();

}
