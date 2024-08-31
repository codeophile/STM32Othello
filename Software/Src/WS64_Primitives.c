/********************************************************************************
 WS64_Primitives.c

 WS2812B LED strips using SPI
 MOSI PA7, pin 13

 XMHz works on LED0Byte/LED1Byte
 11 and 12MHz fails on 40/7C
 10MHz=100nS works on 40/7C
 8MHz=125nS works on 40/7C
 6MHz=167nS works on 60/7C and 40/7C
 5MHz=200nS works on 60/70, 7F (stopped checking in-between), note that LED0Byte of 70 (600nS) failed
 5MHz=200nS works on 40/70, 78, 7C, 7E or 7F
 4MHz=250nS works on 40/60, 70, 78, 7C, 7E or 7F, note that LED0Byte of 60 (500nS) failed
 PREFERRED is 5MHz = 200nS, 0H/L is 200/1400 or 400/1200 | 1H/L is 600/1000 to 1400/200

 ********************************************************************************/

#include "APP.h"
#include "main.h"

//WARNING: Don't set MSBit since MOSI idle follows it (odd I know) and
//high idle state causes WS2812B to fail.
//For 5 MHz (80MHz clock, divide by 16 SPI prescaler)
//0H is 200nS, 1H is 1000nS, period is 1600nS
#define LED0Byte 0x40
#define LED1Byte 0x7C;

//For SPI driver
HAL_StatusTypeDef status;
extern SPI_HandleTypeDef hspi1;


//int LEDBuf[8][8];
//int *pLEDBuf = (int*) LEDBuf;

//I determined 10 distinct colors + dark/off = 11 colors
#define COLOR_CNT 11
int colors[COLOR_CNT];

//spiBuf needs 40 bit reset + 3 color bytes for each LED: 40+64*3*8 = 1576
#define SPIBUF_RESET_LEN 40
#define SPIBUF_DATA_LEN 1536
#define SPIBUF_TOTAL_LEN 1576

uchar spiBuf[SPIBUF_TOTAL_LEN];
uchar *pSPIBuf = (uchar*) spiBuf;

//void StoreLEDBuf(); //temporary
//void StoreSPIBuf();
int CutBrightness(int color, uchar factor);

//----------------------------------------------------------------------------------------------------------
void WS64_Init() {

	//INITIALIZE COLORS
	//Stored in G-R-B
	colors[0] = 0x000000; //off
	colors[1] = 0x003200; //red
	colors[2] = 0x143C00; //orange
	colors[3] = 0x202000; //yellow
	colors[4] = 0x301000; //lime
	colors[5] = 0x400000; //green
	colors[6] = 0x340020; //turquoise
	colors[7] = 0x000080; //blue
	colors[8] = 0x001430; //purple
	colors[9] = 0x0D280D; //pink
	colors[10] = 0x282828; //white

	//Reduce brightness
	for (int cnt = 1; cnt < COLOR_CNT; cnt++)
		colors[cnt] = CutBrightness(colors[cnt], 10);

	/*
	//FOR TESTING: Display all colors
	uchar tempForTest[64];
	uchar cX = 1;
	for (int cnt = 0; cnt < 64; cnt++) {
		tempForTest[cnt] = cX++;
		if (cX == 9) cX = 1;
	}
	WS64_Display(tempForTest);
	*/

}

//----------------------------------------------------------------------------------------------------------
int CutBrightness(int color, uchar factor) {

	uchar grn = ByteNofVal(color, 2);
	uchar red = ByteNofVal(color, 1);
	uchar blu = ByteNofVal(color, 0);

	ByteNofVal(color,2)=grn/factor;
	ByteNofVal(color,1)=red/factor;
	ByteNofVal(color,0)=blu/factor;

	return color;
}

//----------------------------------------------------------------------------------------------------------
void WS64_BlankDisplay() {

	uchar *buf = pSPIBuf;

	int count = SPIBUF_RESET_LEN;
	while (count--) {
		*buf++ = 0;
	}

	count = SPIBUF_DATA_LEN;
	while (count--) {
		*buf++ = LED0Byte;
	}
	status = HAL_SPI_Transmit(&hspi1, &spiBuf[0], SPIBUF_TOTAL_LEN, HAL_MAX_DELAY);

}
//----------------------------------------------------------------------------------------------------------
void WS64_Display(uchar *p64ColorIndexes) {

	int bitX;
	uchar *buf = pSPIBuf;
	int color;
	int cnt;

	//Initialize with 50uS reset.
	//At SPI rate of 6MBits, each byte takes 8/6uS = 1.333uS
	//For 50uS reset (hold MOSI low) period, need 50/1.333 = 37.5. Make it 40.
	//Note: measurements by some sources indicate that only about 9uS are actually needed.
	for (cnt = 0; cnt < SPIBUF_RESET_LEN; cnt++) {
		//Stores SPIBUF_RESET_LEN bytes (40)
		*buf++ = 0;
	}
	for (cnt = 0; cnt < 64; cnt++) {
		//Load color from caller's index
		color = colors[*p64ColorIndexes++];
		//Each color requires 24 bits each of which is represented by one spi byte.
		//I tested to make sure that this loop iterates exactly 24 times.
		//Stores 24*64 bytes = 1536.
		for (bitX = 0x800000; bitX != 0; bitX = bitX >> 1) {
			if ((bitX & color) == 0) {
				*buf++ = LED0Byte;
			} else {
				*buf++ = LED1Byte
				;
			}
		}
	}

	//Display using the BLOCKING variant (so fast (~4 mS for 64 LEDs) it doesn't matter)
	status = HAL_SPI_Transmit(&hspi1, &spiBuf[0], SPIBUF_TOTAL_LEN, HAL_MAX_DELAY);

}


