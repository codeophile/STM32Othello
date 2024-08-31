/********************************************************************************
WS64_Cmd.h
********************************************************************************/

#ifndef INC_WS64_DRAW_H_
#define INC_WS64_DRAW_H_

void WS64_Init();
void WS64_CmdListClear();
void WS64_CmdListAdd(uchar x0, uchar x1, uchar x2, uchar x3, uchar x4, uchar x5, uchar x6);
void WS64_CmdListStart(uchar* initialBoard);
void WS64_CmdListCancel();
void WS64_CmdListUpdate();
void WS64_CmdListTest();
void WS64_Display(uchar* p64ColorIndexes);
void WS64_BlankDisplay();
void WS64_CmdListShowScore(int comp, int opp);
void WS64_CmdStartShow();
void WS64_DisplayChr(uchar nFontChrX);
//void FillBoard(uchar colorX);

#define CMDLISTSIZE 50
#define CMDSIZE 7

enum Colors {
	//Correspond to colors[] defined in WS64_Basics.c
	ecoOff, ecoRed, ecoOrg, ecoYel, ecoLim, ecoGrn,
	ecoTrq, ecoBlu, ecoPur, ecoPnk, ecoWht, ecoCOUNT,
};

 struct ScrollChars{
	//Char data
	uchar charCount;
	uchar chars[20];
	uchar charColors[20];
	//Settings
	uchar fontWidth;
	uchar scrollOffColCnt;
	uchar delay;
	//Update tracking
	uchar charX;
	uchar colX;
};
 struct Snake {
   uchar colorX;
   uchar delay;
   uchar length;
   //rearX needs to go negative
   schar rearX;
   //pointer to a "const uchar"
   const uchar* pMap;
   uchar mapLength;
   uchar effectX;
   uchar effectColorX;
 };

 struct Wave {
 	uchar count;
 	uchar drawX;
 	uchar spacing;
 	uchar thickness;
 	uchar foreColor;
 	uchar backColor;
 	uchar delay;
 };


#endif /* INC_WS64_DRAW_H_ */
