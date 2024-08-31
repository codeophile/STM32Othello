/********************************************************************************
APP.h
********************************************************************************/

#ifndef SRC_APP_H_
#define SRC_APP_H_

//STANDARD ITEMS ------------------------------------------------------------------------
//Surprisingly, char is unsigned by default on this compiler.
//short, int, and long are all signed by default.
typedef unsigned char uchar;
typedef signed char schar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

//Note: Arm Cortex and 8086 are little endian (LSByte stored first). 8051 is big endian.
#define ByteNofVal(Val,N) ((uchar *)&Val)[N]
#define WordNofVal(Val,N) ((ushort *)&Val)[N]

//#include "main.h"
#include <stdbool.h>

void APP_Main();
void GEN_GetDeltaCycles_INIT(void);
uint GEN_GetDeltaCycles(void);


//NON-STANDARD ITEMS ---------------------------------------------------------------------



//#define SetLED GPIOB->BSRR = (uint32_t)GPIO_PIN_3
//#define ClrLED GPIOB->BRR = (uint32_t)GPIO_PIN_3
//#define SetLED3 HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, 1);
//consider GPIOA->BSRR = (1<<5)
#define setLED GPIOB->ODR |= GPIO_PIN_7
#define clrLED GPIOB->ODR &= ~GPIO_PIN_7
#define tglLED GPIOB->ODR ^= GPIO_PIN_7
#define getLED (GPIOB->IDR & GPIO_PIN_7)



#endif /* SRC_APP_H_ */
