/********************************************************************************
 WS64_Cmd.c
 ********************************************************************************/

#include "APP.h"
#include "WS64_Cmd.h"
#include "WS64_Data.h"

//#include <stdlib.h>
//#include <string.h> //for memcpy

extern int cmdListCount;
extern uchar cmdList[CMDLISTSIZE][CMDSIZE];
extern int colors[11];

//The board is what is ultimately drawn on each update.
//It can be accessed either by [row][col] or by index 0-64
uchar board[8][8];
uchar *pBoard = (uchar*) board;

//Currently only used for scrolling in
uchar board2[8][8];
uchar *pBoard2 = (uchar*) board2;

//Update state
int upState;
int upCmdX;
int upDelay; //can go negative
int upStateAfterDelay;
int upCmdX_LoopBack;
int loopCnt;
bool WS64CmdListCompleteFlag;
bool cmdInitComplete;

//Vars for tracking draw state
struct Snake snake;
struct ScrollChars scrollChars;
struct Wave wave;
int flashCnt;
int rainbowCnt;
int scrollOffCnt;

//Used by Box cmd to draw 4 lines
uchar subCmd[CMDSIZE];

void FillBoard(uchar colorX);
int Line(uchar cmd[]);
int Snake();
void SnakeEffect(int ledX);

int Pattern(uchar cmd[]);
int ProcessCmd(uchar cmd[]);
void Load8Bytes(uchar data[], uchar colorX);
int InitScrollTextData(uchar cmd[]);
int ScrollText();
int ScrollChars();

void ClearColumn(int colX);
void LoadColumn(int colX, uchar colData, uchar colorX);
void ShiftLeftOneColumn();

void DrawFullDiagDR(int startX, uchar colorX);
void DrawDWave();
void ShiftDiagUR();
void Rainbow();



//------------------------------------------------------------------------------
void GoToNextCmd() {

//This function makes sure that init is always set false before moving to next cmd
	cmdInitComplete = false;
	upCmdX++;
}

//------------------------------------------------------------------------------
void WS64_CmdListStart(uchar *initialBoard) {

	if (initialBoard == 0) {
		//Clear board?
		for (int cnt = 0; cnt < 64; cnt++)
			pBoard[cnt] = 0;
	} else {
		//Load callers board
		for (int cnt = 0; cnt < 64; cnt++)
			pBoard[cnt] = initialBoard[cnt];
	}

	WS64CmdListCompleteFlag = false;
	upState = 1;
}
//------------------------------------------------------------------------------
void WS64_CmdListCancel() {

	//Clear display and cancel further list processing
	FillBoard(0);
	WS64_Display(pBoard);
	upState = 0;

}
//------------------------------------------------------------------------------
void WS64_CmdListUpdate() {

//Called at 100 HZ
	int processCmdReturn;
	switch (upState) {

	case 0:
		//Startup waiting for start
		break;

	case 1:
		//Initialize for first command
		processCmdReturn = 0;
		upCmdX = 0;
		cmdInitComplete = false;
		upState = 2;

	case 2:
		//Running command list
		while (1) {
			//if(upCmdX >= sizeof(cmdList)/BYTESPERCMD){
			if (upCmdX >= cmdListCount) {
				WS64CmdListCompleteFlag = true;
				upState = 0;
				break;
			}
			//PROCESS COMMAND
			processCmdReturn = ProcessCmd(cmdList[upCmdX]);
			//HANDLE RETURN VALS
			//<0 error
			//0 next command
			//1 next command after delay
			//2 repeat ALL commands after delay
			//3 same command after delay
			//4 delay without display
			if (processCmdReturn < 0) {
				//cmd error, ignoring, move on to next cmd without display or delay
				//USEFUL TO SET BREAKPOINT HERE WHEN DEBUGGING
				GoToNextCmd();
			} else if (processCmdReturn == 0) {
				//do next command without display or delay
				GoToNextCmd();
			} else if (processCmdReturn == 1) {
				//do next cmd after display and delay
				GoToNextCmd();
				if (upDelay > 0) {
					//May be using SETONE to individually set positions and don't want a display until done
					//display and delay
					WS64_Display(pBoard);
					upStateAfterDelay = 2;
					upState = 11;
					break;
				}
			} else if (processCmdReturn == 2) {
				//delay then repeat this cmd, i.e., don't increment upCmdX
				//(used during flashing)
				upStateAfterDelay = 2;
				upState = 11;
				break;
			} else if (processCmdReturn == 3) {
				//display and delay then repeat this cmd, i.e., don't increment upCmdX
				//(used during snake and scroll)
				WS64_Display(pBoard);
				upStateAfterDelay = 2;
				upState = 11;
				break;
			} else if (processCmdReturn == 4) {
				//Delay only before moving on to next cmd - used only for command 99
				GoToNextCmd();
				upStateAfterDelay = 2;
				upState = 11;
				break;
			} else {
				//coding error, ignoring, do next cmd without display or delay
				GoToNextCmd();
			}
		}
		break;

	case 11:
		if (--upDelay <= 0) {
			upState = upStateAfterDelay;
		}
		break;

	}					//switch
}

//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
int ProcessCmd(uchar cmd[]) {

//RETURN VALS
//<0 error
//0 next command
//1 next command after delay
//2 repeat ALL commands after delay
//3 same command after delay

	int ret;

	switch (cmd[0]) {
	case 1:
		//FILL: 1,CX,D
		if (cmd[1] >= ecoCOUNT) return -1;
		//WSLED_FillLEDBuf(color);
		FillBoard(cmd[1]);
		upDelay = cmd[2];
		return 1;
	case 2:
		//SET ONE: 2,R,C,CX,D
		if (cmd[3] >= ecoCOUNT) return -3;
		if (cmd[1] > 7) return -1;
		if (cmd[2] > 7) return -2;
		board[cmd[1]][cmd[2]] = cmd[3];
		upDelay = cmd[4];
		return 1;
	case 3:
		//PATTERN: 3,patX,CX,D
		FillBoard(0);
		uint size = sizeof(pattern) / 8;
		if (cmd[1] >= size) return -1;
		if (cmd[2] >= ecoCOUNT) return -2;
		Load8Bytes(pattern[cmd[1]], cmd[2]);
		upDelay = cmd[3];
		return 1;
	case 4:
		if (cmdInitComplete == false) {
			scrollChars.colX = 0;
			scrollChars.charX = 0;
			cmdInitComplete = true;
		}
		ret = ScrollChars();
		upDelay = scrollChars.delay;
		return ret;
		break;
	case 5:
		//SNAKE: 5,MapX,L,CX,D,effX,CX
		if (cmdInitComplete == false) {
			//INITIALIZE
			//needs error checking
			//Check color
			if (cmd[3] >= ecoCOUNT) return -1;
			snake.colorX = cmd[3];
			//Clamp length at 6 (arbitrary)
			snake.length = cmd[2];
			if (snake.length > 6) snake.length = 6;
			//Initialize rearX
			snake.rearX = 1 - snake.length;
			//Select map
			switch (cmd[1]) {
			//This method allows maps to be of varying lengths
			default:
				return -2;
			case 0:
				snake.pMap = snakeMap0;
				snake.mapLength = sizeof(snakeMap0);
				break;
			case 1:
				snake.pMap = snakeMap1;
				snake.mapLength = sizeof(snakeMap1);
				break;
			case 2:
				snake.pMap = snakeMap2;
				snake.mapLength = sizeof(snakeMap2);
				break;
			case 3:
				snake.pMap = snakeMap3;
				snake.mapLength = sizeof(snakeMap3);
				break;
			}
			//Check effectX
			snake.effectX = cmd[5];
			//Check effect color
			if (cmd[6] >= ecoCOUNT) return -3;
			snake.effectColorX = cmd[6];
			cmdInitComplete = true;
		}
		ret = Snake();
		upDelay = cmd[4];
		return ret;

	case 6:
		//FLASH	6,onD,offD,cnt (odd leaves blank, even leaves on)
		if (cmdInitComplete == false) {
			//INITIALIZE
			//Multiplying by 2 because "flash" means on/off.
			//Other effect of this is that it's always left in the off state.
			flashCnt = 2 * (int) cmd[3];
			cmdInitComplete = true;
		}
		if (flashCnt-- == 0) return 0;
		if (flashCnt % 2) {
			//flashCnt is odd
			WS64_Display(pBoard);
			upDelay = cmd[1];
		} else {
			WS64_BlankDisplay();
			upDelay = cmd[2];
		}
		//Return to this command after delay
		return 2;

	case 7:
		//RAINBOW: 7,cnt,D
		if (cmdInitComplete == false) {
			rainbowCnt = cmd[1];
			cmdInitComplete = true;
		}
		if (rainbowCnt-- == 0) return 0;
		//Return to this command after delay
		Rainbow();
		upDelay = cmd[2];
		return 3;
	case 8:
		//DWAVE: 8,cnt,spacing,thickness,fcX,bcX,D
		if (cmdInitComplete == false) {
			wave.count = cmd[1];
			wave.spacing = cmd[2];
			wave.thickness = cmd[3];
			if (cmd[4] >= ecoCOUNT) return -1;
			if (cmd[5] >= ecoCOUNT) return -2;
			wave.foreColor = cmd[4];
			wave.backColor = cmd[5];
			wave.delay = cmd[6];
			wave.drawX = 0;
			cmdInitComplete = true;
		}
		if (wave.count-- == 0) return 0;
		//WSLED_FillLEDBuf(wave.backColor);
		DrawDWave();
		Rainbow();
		upDelay = wave.delay;
		return 3;
	case 9:
		//SCROLLOFF 9,cnt,D (scrolls in cnt columns of blank)
		if (cmdInitComplete == false) {
			scrollOffCnt = cmd[1];
			cmdInitComplete = true;
		}
		if (scrollOffCnt-- == 0) return 0;
		ShiftLeftOneColumn();
		ClearColumn(7);
		upDelay = cmd[2];
		return 3;
	case 10:
		//SCROLL IN 10,D
		//Scrolls in board2
		if (cmdInitComplete == false) {
			scrollOffCnt = 8;
			cmdInitComplete = true;
		}
		if (scrollOffCnt-- == 0) return 0;
		ShiftLeftOneColumn();
		for (int rowX = 0; rowX < 8; rowX++)
			board[rowX][7] = board2[rowX][7 - scrollOffCnt];
		upDelay = cmd[1];
		return 3;
	case 11:
		case 12:
		case 13:
		case 14:
		//LINE-hr:	11,R,C,CX,L,D
		//LINE-vd:	12,R,C,CX,L,D
		//LINE-ddr:	13,R,C,CX,L,D
		//LINE-dur:	14,R,C,CX,L,D
		//Must set delay tick here since Line() is also called by BOX cmd
		upDelay = cmd[5];
		ret = Line(cmd);
		if (ret < 0) return ret;
		return 1;
	case 15:
		//BOX: 15,R,C,CX,H,W,T
		//Note that line delays are ignored
		//Draws 4 lines
		//Line: cmdX,R,C,CX,L,D
		upDelay = cmd[6];
		//TOP HORIZ LINE
		subCmd[0] = 11;
		subCmd[1] = cmd[1]; //row=row
		subCmd[2] = cmd[2]; //col=col
		subCmd[3] = cmd[3]; //CX=CX
		subCmd[4] = cmd[5]; //L=W
		ret = Line(subCmd);
		if (ret < 0) return ret;
		//BOTTOM HORIZ LINE
		//Add height to row
		subCmd[1] += (cmd[4] - 1);
		ret = Line(subCmd);
		if (ret < 0) return ret - 100;
		//LEFT VERT LINE
		//Nothing to draw if height <3
		if (cmd[4] < 3) return 1;
		subCmd[0] = 12;
		subCmd[1] = cmd[1] + 1;
		subCmd[4] = cmd[4] - 2;
		ret = Line(subCmd);
		if (ret < 0) return ret - 100;
		//RIGHT VERT LINE
		//Nothing to draw if width <3
		if (cmd[5] < 2) return 1;
		//Add width to column
		subCmd[2] += (cmd[5] - 1);
		ret = Line(subCmd);
		if (ret < 0) return ret - 100;
		upDelay = cmd[6];
		return 1;
	case 16:
		//Copy board to board 2
		for (int cnt = 0; cnt < 64; cnt++)
			pBoard2[cnt] = pBoard[cnt];
		return 0;
	case 20:
		//LOOPINIT	20,cnt
		//Set loop back to this cmdX and "return 0" in command 21 increments it by 1
		upCmdX_LoopBack = upCmdX;
		//Initialize loop counter
		loopCnt = cmd[1];
		return 0;
	case 21:
		//LOOPBACK	21
		if (--loopCnt > 0) upCmdX = upCmdX_LoopBack;
		return 0;
	case 99:
		//Delay only (useful only after scroll or snake)
		upDelay = cmd[1];
		return 4;
	case 100:
		//Display and Delay
		upDelay = cmd[1];
		return 1;

	default:
		return -99;
	}
	return 0;

}

//------------------------------------------------------------------------------
void FillBoard(uchar colorX) {
	uchar *pbrd = pBoard;
	for (int cnt = 0; cnt < 64; cnt++)
		*pbrd++ = colorX;
}

//------------------------------------------------------------------------------
void Rainbow() {

	for (int row = 0; row < 8; row++) {
		for (int col = 0; col < 8; col++) {
			if (board[row][col] != 0) {
				board[row][col] = row + 1;
			}
		}
	}
}

//------------------------------------------------------------------------------
void ShiftDiagUR() {

//Shifts diagonals up and to the right
//Applies color of left/topmost start of diagonal to entire
	uchar colorX;
	for (int col = 6; col >= 0; col--) {
		colorX = board[0][col];
		DrawFullDiagDR(-(col + 1), colorX);
	}
	for (int row = 1; row < 8; row++) {
		colorX = board[row][0];
		DrawFullDiagDR(row - 1, colorX);
	}

}
//------------------------------------------------------------------------------
void DrawDWave() {

//Draws diagonals down and to the right
//Positive startX indicates start row
//Negative startX indicates start col
//Starting with row 7 and decrementing by spacing, each diagonal's startX moves
//up to the top row and then to the right along columns

	ShiftDiagUR();

	if (wave.drawX < wave.thickness) {
		pBoard[56] = wave.foreColor;
		wave.drawX++;
	} else if (wave.drawX < wave.thickness + wave.spacing) {
		pBoard[56] = wave.backColor;
		wave.drawX++;
		if (wave.drawX >= wave.thickness + wave.spacing) wave.drawX = 0;
	}
}

//------------------------------------------------------------------------------
void DrawFullDiagDR(int startX, uchar colorX) {

//Draws diagonal down and to the right
//Positive startX indicates start row
//Negative startX indicates start col

//Get startX
	int row, col;
	if (startX >= 0) {
		row = startX;
		col = 0;
	} else {
		row = 0;
		col = -startX;
	}
	while (row < 8 && col < 8) {
		board[row++][col++] = colorX;
	}

}

//------------------------------------------------------------------------------
int ScrollChars() {

//	uchar charX;
//	uchar fontWidth;
//	uchar colX;

//Check if scroll is finished.
	if (scrollChars.charX >= scrollChars.charCount) {
		//Character data is finished.
		//Now scroll 8 bits to scroll off screen.
		//scrollTextData.colCnt on first pass = scrollTextData.fontWidth
		if (scrollChars.colX++ >= scrollChars.scrollOffColCnt) {
			//Set scrollOffColCnt to 7 scroll off entirely
			//Set to 0 for no scrolling. At 0, col 7 will be blank due to padding after each char
			//Done scrolling off
			return 0;
		} else {
			//scrolling off
			ShiftLeftOneColumn();
			ClearColumn(7);
			return 3;
		}
	}
//ShiftLeftOneColumn() leaves col7 unchanged. Must overwrite here.
	ShiftLeftOneColumn();
	if (scrollChars.colX < scrollChars.fontWidth) {
		uchar chr = scrollChars.chars[scrollChars.charX];
		uchar col = nFont[chr - 32][scrollChars.colX++];
		LoadColumn(7, col, scrollChars.charColors[scrollChars.charX]);

	} else {
		//Pad with one blank column (shift left doesn't clear col 7)
		ClearColumn(7);
		scrollChars.charX++;
		scrollChars.colX = 0;
	}

//Do delay then continue with scroll command
	return 3;

}
//------------------------------------------------------------------------------
void ClearColumn(int colX) {

//Used for scrolling
	for (int rowX = 0; rowX < 8; rowX++) {
		board[rowX][colX] = 0;
	}
}
//------------------------------------------------------------------------------
//void LoadCol7FromBoard2(int srcCol){
//
//	for(int rowX=7; rowX>=0; rowX--){
//		board[rowX][srcCol]=board2[rowX][colX];
//
//	}
//
//}

//------------------------------------------------------------------------------
void LoadColumn(int colX, uchar colData, uchar colorX) {

//Only used for text display used by scrolling

//Originally, I initialized bit to 0x80, but that bit was always 0 (7 pixel height) leaving the bottom row blank.
//So I modified it to start with 0x40 so letters use the bottom row and instead leave the top row blank.
//I still let it write the top row (which is always 0) in case user doesn't start with a blank screen.
	int bit = 0x40;
	for (int rowX = 7; rowX >= 0; rowX--) {
		if (bit & colData) {
			board[rowX][colX] = colorX;
		} else {
			board[rowX][colX] = 0;
		}
		bit = bit >> 1;
	}
}

//------------------------------------------------------------------------------
void ShiftLeftOneColumn() {

//Used for horiz scrolling
//Shifts columns 1-7 one column to the left.
//Column 7 remains unchanged, assumes caller will overwrite.

	for (int colX = 0; colX < 7; colX++) {
		for (int rowX = 0; rowX < 8; rowX++) {
			board[rowX][colX] = board[rowX][colX + 1];
		}
	}
}

//------------------------------------------------------------------------------
void Load8Bytes(uchar data[], uchar colorX) {

	int rowX;
	int bitX;
	uchar colData;
	for (uint colX = 0; colX < 8; colX++) {
		bitX = 0x80;
		colData = data[colX];
		for (rowX = 7; rowX >= 0; rowX--) {
			if (bitX & colData) {
				board[rowX][colX] = colorX;
			} else {
				board[rowX][colX] = 0;
			}
			bitX = bitX >> 1;
		}
	}
}

//------------------------------------------------------------------------------
int Snake() {

//Snake has variable length.
//rearX marks the rear most pixel of the snake.
//The index of the "head" of the snake is rearX+length-1
//Initially, rearX is set to 1-length such that the head of the snake is the first pixel to be drawn.
//As code begins to draw the snake, the rear pixel isn't shown until rearX increments to 0
//at which time the entire snake has been drawn.
//As code finishes the run through the map, it doesn't display "head" pixels that have run off the map.
//Code doesn't finish drawing until the rear has run off the map.

//Check if snake is finished.
//Waited until now for last pixel to have been displayed for delay period.
	if (snake.rearX >= snake.mapLength) {
		//Clear the "screen"
		FillBoard(0);
		//The rear pixel has run off map.
		//Proceed to next command with no delay
		return 0;
	}

//Clear the "screen"
	FillBoard(0);

	uint pixelCount = snake.length;
//Start drawing with the rear of snake
	int mapX = snake.rearX;
	while (pixelCount--) {
		if (mapX >= snake.mapLength) {
			//The remainder (head portions) of the snake has run off the map
			break;
		}
		if (mapX >= 0) {
			//This pixel of the snake is on the map
			uchar ledX = snake.pMap[mapX];
			//pLEDBuf[ledX]=snake.color;
			pBoard[ledX] = snake.colorX;
			if (snake.effectX > 0) {
				SnakeEffect(ledX);
			}
		}
		//Incrementing from rear to head
		mapX++;
	}

//Increment snake position
	snake.rearX++;

//Do delay then continue with snake command
	return 3;

}

//------------------------------------------------------------------------------
void SnakeEffect(int ledX) {

	int row = ledX / 8;
	int col = ledX - (row * 8);

//Mirror left to right
//int newCol= 7-col;
//int newLedX=row*8+newCol;

//Mirror diagonally
	int newCol = 7 - col;
	int newRow = 7 - row;
	int newLedX = newRow * 8 + newCol;

//pLEDBuf[newLedX]=snake.colorEff;
	pBoard[newLedX] = snake.effectColorX;

}

//------------------------------------------------------------------------------
int Line(uchar cmd[]) {

//LINE-hr:	11,R,C,CX,L,D
//LINE-vd:	12,R,C,CX,L,D
//LINE-ddr:	13,R,C,CX,L,D
//LINE-dur:	14,R,C,CX,L,D

//Get and validate cmd data
	uchar command = cmd[0];
	uchar row = cmd[1];
	if (row > 7) return -1;
	uchar col = cmd[2];
	if (col > 7) return -2;
	uchar colorX = cmd[3];
	if (colorX >= ecoCOUNT) return -3;
	uchar len = cmd[4];
	if (len < 1) return -4;
	if (len > 8) return -5;

	switch (command) {

	case 11:
		//Horiz Line Right
		if ((col + len) > 8) return -6;
		while (len--) {
			board[row][col++] = colorX;
		}
		return 1;
	case 12:
		//Vert Line Down
		if ((row + len) > 8) return -7;
		while (len--) {
			board[row++][col] = colorX;
		}
		return 1;
	case 13:
		//Diag Line Down & Right
		if ((row + len) > 8) return -8;
		if ((col + len) > 8) return -9;
		while (len--) {
			board[row++][col++] = colorX;
		}
		return 1;
	case 14:
		//Diag Line Up & Right
		if ((row - len) < -1) return -10;
		if ((col + len) > 8) return -11;
		while (len--) {
			board[row--][col++] = colorX;
		}
		return 1;
	default:
		return -12;

	}
}
//------------------------------------------------------------------------------
void WS64_DisplayChr(uchar nFontChrX) {

	//CREATED TO SUPPORT MODE SET

	//nFontChrX starts with blank since control chrs aren't stored.
	//Caller can supply ASCII - 32.
	//Or, if caller wants number, number + 16

	uchar col[8];
	col[0] = 0;
	col[1] = 0;
	col[2] = nFont[nFontChrX][0] << 1;
	col[3] = nFont[nFontChrX][1] << 1;
	col[4] = nFont[nFontChrX][2] << 1;
	col[5] = nFont[nFontChrX][3] << 1;
	col[6] = nFont[nFontChrX][4] << 1;
	col[7] = 0;

	Load8Bytes(col,ecoBlu);
	WS64_Display(pBoard);

}
