/********************************************************************************
SLED.c - Serial LEDs

Datasheets for both APA102 and SK9822 say to end with 32x1
Start frame: 32x0
LED frame (111)+(nnnnn) global brightness, 8 bit BLUE, 8 bit GREEN, 8 bit RED
End frame: 32x1

According to a Hackaday guy who did some thorough testing, the following will work for either SK9822 or APA107
Start frame: 32x0
LED frame (111)+(nnnnn) global brightness, 8 bit BLUE, 8 bit GREEN, 8 bit RED
End frame: 32x0 + minimum of LEDCount/2 more zeros.
I would think a multiple of 32 is best for the additional zeros, so I will do 64x0


********************************************************************************/

#include "APP.h"
#include "main.h"

void SLED_32x0();
void SLED_32x1();
void SetColor(uint color);

//BIT BANG GPIO MACROS
//Othello board: LCLK PH3, LDAT PB7
//Board pin D12, yellow wire on LED string
#define SetSCK GPIOH->BSRR = (uint32_t)GPIO_PIN_3
#define ClrSCK GPIOH->BRR = (uint32_t)GPIO_PIN_3

//Board pin D11, green wire on LED string
#define SetSDO GPIOB->BSRR = (uint32_t)GPIO_PIN_7
#define ClrSDO GPIOB->BRR = (uint32_t)GPIO_PIN_7

//First byte is global brightness. The first 3 bits are always ones then the last 5 bits (0-31) are the brightness.
//I start each byte with 'E' such that the MSB of brightness is 0 - already at half brightness.
//So, we only have to consider the second digit as brightness.
//I want all global brightness bytes to be the same.

//Color indexes manipulated to match colors defined in WS64_Basics
uint scolors[11]={
	0xE0000000, //off=0
	0xE1000080, //red=1
	0xE0000000, //filler
	0xE1002080, //yellow=3
	0xE0000000, //filler
	0xE1002000, //green=5
	0xE0000000, //filler
	0xE1FF0000, //blue=7
	0xE0000000, //filler
	0xE0000000, //filler
	0xE0000000, //filler
};

//----------------------------------------------------------------------------------------------------------
void SLED_DisplayStatus(uchar colorX[]){

	//Start frame
	SLED_32x0();

	SetColor(scolors[colorX[0]]);
	SetColor(scolors[colorX[1]]);
	SetColor(scolors[colorX[2]]);

	//End frame
	SLED_32x0();
	//SLED_32x0();

}

//----------------------------------------------------------------------------------------------------------
void SLED_Test(){

	//Start frame
	SLED_32x0();

	SetColor(scolors[0]);
	SetColor(scolors[7]);
	SetColor(scolors[0]);

	//End frame
	SLED_32x0();
	//SLED_32x0();

}
//----------------------------------------------------------------------------------------------------------
void SetColor(uint color){

	//SCK should aready be clear, but just in case..
	ClrSCK;
	uint bit = 0x80000000;
	while(bit>0){
		if(color & bit)SetSDO; else ClrSDO;
		SetSCK; ClrSCK;
		bit=bit>>1;
	}
}

void SLED_32x0(){
	ClrSDO;
	for(int cnt=0; cnt<32; cnt++){
		SetSCK; ClrSCK;
	}
}
void SLED_32x1(){
	SetSDO;
	for(int cnt=0; cnt<32; cnt++){
		SetSCK; ClrSCK;
	}
	ClrSDO;
}


