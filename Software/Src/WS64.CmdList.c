/********************************************************************************
WS64_CmdList.c

- Use functions below to create list of drawing commands
- Call WS64_CmdListStart() to start drawing.
- On each update, call WS64_CmdListUpdate()
- Monitor WS64CmdListCompleteFlag to

********************************************************************************/

#include "App.h"
#include "WS64_Cmd.h"
#include <string.h> //for memcpy

extern struct ScrollChars scrollChars;

uchar cmdList[CMDLISTSIZE][CMDSIZE];
int cmdListCount=0;


//COMMANDS R=Row, C=Column, CX=colorX, D=delay tics
//Line directions: h=hor, v=vert, u=up, d=Down,
//FILL:		1,CX,D
//SET ONE:	2,R,C,CX,D
//PATTERN:	3,patX,CX,D
//CHARS:	4,D,ScrollOffCols
//SNAKE: 	5,MapX,L,CX,D,effX,CX
//FLASH:	6,onD,offD,cnt (odd leaves blank, even leaves on)
//RAINBOW: 	7,cnt,D
//DWAVE:	8,cnt,spacing,thickness,fcX,bcX,D
//SCROLLOFF 9,cnt,D (scrolls in cnt columns of blank)
//SCROLLIN  10,D (scrolls in board2)
//LINE-hr:	11,R,C,CX,L,D
//LINE-vd:	12,R,C,CX,L,D
//LINE-ddr:	13,R,C,CX,L,D
//LINE-dur:	14,R,C,CX,L,D
//BOX:		15,R,C,CX,H,W,D
//COPY;		16 (copies board to board2)
//LOOPINIT	20,cnt
//LOOPBACK	21
//DELAY		99,D (doesn't display)
//DISPLAY	100,D (display then delay)

//0	    1	2	3	4	5	6	7
//8	    9	10	11	12	13	14	15
//16	17	18	19	20	21	22	23
//24	25	26	27	28	29	30	31
//32	33	34	35	36	37	38	39
//40	41	42	43	44	45	46	47
//48	49	50	51	52	53	54	55
//56	57	58	59	60	61	62	63

//------------------------------------------------------------------------------
void WS64_CmdListClear(){
	cmdListCount=0;
}

//------------------------------------------------------------------------------
void WS64_CmdListAdd(uchar x0, uchar x1, uchar x2, uchar x3, uchar x4, uchar x5, uchar x6){

	cmdList[cmdListCount][0]=x0;
	cmdList[cmdListCount][1]=x1;
	cmdList[cmdListCount][2]=x2;
	cmdList[cmdListCount][3]=x3;
	cmdList[cmdListCount][4]=x4;
	cmdList[cmdListCount][5]=x5;
	cmdList[cmdListCount][6]=x6;
	cmdListCount++;
}

//------------------------------------------------------------------------------
void WS64_CmdListShowScore(int comp, int opp){

	//STORE SCORES AND SET UP FOR SCROLLING
    //Computer score
	int arrX=0;
    if (comp <= 9) {
    	scrollChars.chars[arrX] = 48 + comp;
    	scrollChars.charColors[arrX++] = ecoBlu;
    }
    else  {
    	scrollChars.chars[arrX] = 48 + comp / 10;
    	scrollChars.charColors[arrX++] = ecoBlu;
        scrollChars.chars[arrX] = 48 + comp % 10;
        scrollChars.charColors[arrX++] = ecoBlu;
    }
    //Insert blank
	scrollChars.chars[arrX] = '-';
	scrollChars.charColors[arrX++] = ecoYel;
    //Opponent score
    if (opp <= 9) {
    	scrollChars.chars[arrX] = 48 + opp;
    	scrollChars.charColors[arrX++] = ecoRed;
    }
    else {
    	scrollChars.chars[arrX] = 48 + opp / 10;
    	scrollChars.charColors[arrX++] = ecoRed;
        scrollChars.chars[arrX] = 48 + opp % 10;
        scrollChars.charColors[arrX++] = ecoRed;
    }
    scrollChars.charCount=arrX;
	scrollChars.fontWidth=5;
	scrollChars.scrollOffColCnt=0;
	scrollChars.delay=15;

	//Starting new list---------------------
	WS64_CmdListClear();

	//copy board to board 2 for scrolling back i
	WS64_CmdListAdd(16,0,0,0,0,0,0);

	//Scroll off 1 column to allow 1 blank col before text starts
	WS64_CmdListAdd(9,1,15,0,0,0,0);

	//Scroll in scores
	WS64_CmdListAdd(4,0,0,0,0,0,0);

	//Scroll in board2
	WS64_CmdListAdd(10,15,0,0,0,0,0);


}
//------------------------------------------------------------------------------
void WS64_CmdStartShow(){

	//Initialize char string
	memcpy(scrollChars.chars,"Othello",7);
	scrollChars.charCount=7;
	for(int cnt=0; cnt<7; cnt++)scrollChars.charColors[cnt]=ecoBlu;
//	scrollChars.charColors[0]=ecoBlu;
//	scrollChars.charColors[1]=ecoRed;
//	scrollChars.charColors[2]=ecoYel;
//	scrollChars.charColors[3]=ecoGrn;
//	scrollChars.charColors[4]=ecoPnk;
//	scrollChars.charColors[5]=ecoTrq;
//	scrollChars.charColors[6]=ecoOrg;
	scrollChars.fontWidth=5;
	scrollChars.scrollOffColCnt=7;
	scrollChars.delay=10;


	//Starting new list---------------------
	WS64_CmdListClear();

	//Clear screen
	WS64_CmdListAdd(1,0,0,0,0,0,0);

	//Diagonal waves
	WS64_CmdListAdd(8,28,4,3,ecoBlu,0,10);
	//Scroll off diagonally
	WS64_CmdListAdd(8,12,4,2,0,0,10);

	//Scroll "Othello"
	WS64_CmdListAdd(4,0,0,0,0,0,0);

	//Snake
	WS64_CmdListAdd(5,1,5,ecoGrn,5,1,ecoYel);
	//Clear screen
	WS64_CmdListAdd(1,0,0,0,0,0,0);

	//2 Loop backs to here
	WS64_CmdListAdd(20,2,0,0,0,0,0);
	//4 color boxes
	WS64_CmdListAdd(15,3,3,1,2,2,10);
	WS64_CmdListAdd(15,2,2,2,4,4,10);
	WS64_CmdListAdd(15,1,1,3,6,6,10);
	WS64_CmdListAdd(15,0,0,4,8,8,10);
	//4 color boxes
	WS64_CmdListAdd(15,3,3,5,2,2,10);
	WS64_CmdListAdd(15,2,2,6,4,4,10);
	WS64_CmdListAdd(15,1,1,7,6,6,10);
	WS64_CmdListAdd(15,0,0,8,8,8,10);
	//4 color boxes
	WS64_CmdListAdd(15,3,3,9,2,2,10);
	WS64_CmdListAdd(15,2,2,10,4,4,10);
	WS64_CmdListAdd(15,1,1,1,6,6,10);
	WS64_CmdListAdd(15,0,0,2,8,8,10);

	//Loop back
	WS64_CmdListAdd(21,0,0,0,0,0,0);

	//Delay
	WS64_CmdListAdd(99,20,0,0,0,0,0);

	//Blank screen from inside out
	//BOX:		15,R,C,CX,H,W,D
	WS64_CmdListAdd(15,3,3,0,2,2,15);
	WS64_CmdListAdd(15,2,2,0,4,4,15);
	WS64_CmdListAdd(15,1,1,0,6,6,15);
	WS64_CmdListAdd(15,0,0,0,8,8,15);

	//Delay
	//WS64_CmdListAdd(99,50,0,0,0,0,0);

	//Scroll off 8 columns
	//WS64_CmdListAdd(9,8,20,0,0,0,0);


//	WS64_CmdListAdd(,,0,0,0,0,0);
//	WS64_CmdListAdd(,,0,0,0,0,0);


}












