/********************************************************************************
 OTH_Primitives.c
 ********************************************************************************/

#include "APP.h"
#include "WS64_Cmd.h"
#include "OTH_Main.h"
#include <stdlib.h> //for random number

//Items here that are not in OTH_Main.h
void StoreFlipListInThisDir(uchar board[], int posX, int dirX, uchar pType);
bool IsPlayableInThisDir(uchar board[], int posX, int dirX, uchar pType);
int GetNextPosInThisDir(int position, int dirX);

/*
 Uses <stdlib.h> which defines:
 #define __RAND_MAX 0x7fffffff
 int	rand (void);
 void	srand (unsigned __seed);
 I COULD USE BATTERY VOLTAGE AS SEED
 srand(x) seeds
 rand() returns 0 to RAND_MAX
 //random int between 0 and 19
 int r = rand() % 20;
 */

//List of playable positions  (for any board and for either computer or opponent)
uchar playableList[PLAY_LIST_SIZE];
int playableListCOUNT;

//List of flips resulting from a play (for any board and for either computer or opponent)
uchar flipList[FLIP_LIST_SIZE];
int flipListCOUNT;

//0	    1	2	3	4	5	6	7
//8	    9	10	11	12	13	14	15
//16	17	18	19	20	21	22	23
//24	25	26	27	28	29	30	31
//32	33	34	35	36	37	38	39
//40	41	42	43	44	45	46	47
//48	49	50	51	52	53	54	55
//56	57	58	59	60	61	62	63

uchar clockWiseX[64] = {
	//Search order when looking for legal moves.
	//Quasi clockwise ordering (imagine a cirle with right going arrow in each quadrant)
	//Quad1 (top left) up as go right
	//Quad2 (top right) down as go right
	//Quad3 (bot right) down as go left
	//Quad4 (bot left) up as go left
	//Rows are Quad1-4
	24, 16, 8, 0, 25, 17, 9, 1, 26, 18, 10, 2, 27, 19, 11, 3, 4, 12, 20, 28, 5, 13, 21, 29, 6, 14, 22, 30, 7, 15, 23,
	31, 39, 47, 55, 63, 38, 46, 54, 62, 37, 45, 53, 61, 36, 44, 52, 60, 59, 51, 43, 35, 58, 50, 42, 34, 57, 49, 41, 33,
	56, 48, 40, 32 };

ArchivedBoard archivedBoards[ARCHIVED_BOARD_COUNT];


//----------------------------------------------------------------------------------------------------------
void StoreFlipList(uchar board[], int posX, uchar pType) {

	flipListCOUNT = 0;
	for (int dirX = 0; dirX < 8; dirX++) {
		StoreFlipListInThisDir(board, posX, dirX, pType);
	}
}
//----------------------------------------------------------------------------------------------------------
void StoreFlipListInThisDir(uchar board[], int posX, int dirX, uchar pType) {

	int initialFlipCount = flipListCOUNT;
	for (;;) {
		posX = GetNextPosInThisDir(posX, dirX);
		if (posX == -1) break; //past edge of board
		if (board[posX] == eptNone) break;
		if (board[posX] == pType) return;
		//Must be owned by opponent. Flip it.
		flipList[flipListCOUNT++] = (uchar) posX;
	}
	//If reached here, we have nothing to flip afterall. So reset flip count
	flipListCOUNT = initialFlipCount;
}
//----------------------------------------------------------------------------------------------------------
void StorePlayableList(uchar board[], uchar pType) {

	//Init
	playableListCOUNT = 0;

	//Find plays
	for (int cnt = 0; cnt < 64; cnt++) {
		int posX = clockWiseX[cnt];
		if (IsPlayable(board, posX, pType)) {
			playableList[playableListCOUNT++] = (uchar) posX;
		}
	}
}

//----------------------------------------------------------------------------------------------------------
bool IsPlayable(uchar board[], int posX, uchar pType) {

	//Note: the line below had been mistakenly coded as boardNow[] instead of board[]
	//Calls affected were from StoreScoreForTrialPos() (other calls were passing boardNow[] anyway.
	if (board[posX] != eptNone) return false;
	for (int dirX = 0; dirX < 8; dirX++) {
		if (IsPlayableInThisDir(board, posX, dirX, pType)) return true;
	}
	return false;
}
//----------------------------------------------------------------------------------------------------------
bool IsPlayableInThisDir(uchar board[], int posX, int dirX, uchar pType) {

	int opponentCount = 0;
	for (;;) {
		posX = GetNextPosInThisDir(posX, dirX);
		if (posX == -1) return false; //past edge of board
		if (board[posX] == eptNone) return false;
		if (board[posX] == pType) return opponentCount > 0;
		//Must be owned by opponent
		opponentCount++;
	}
}
//----------------------------------------------------------------------------------------------------------
int GetNextPosInThisDir(int position, int dirX) {

	//IMPORTANT
	//Directions are indexed 0-7
	//This is the ONLY function that gives meaning to the direction because ALL of its callers
	//are simply iterating through all directions. They don't care what direction 0-7 correspond to.

	//BOARD POSITIONS ARE INDEXED FROM 0-63
	//Row 0 is on top, from left to right are positions 0-7, next row down are positions 8-15, etc.

	//Get row and column of caller's position
	int r = position / 8;
	int c = position % 8;

	switch (dirX) {
	//UP
	case 0:
		r--;
		break; //U
	case 1:
		r--;
		c++;
		break; //UR
	case 2:
		r--;
		c--;
		break;  //UL
		//DOWN
	case 3:
		r++;
		break; //D
	case 4:
		r++;
		c++;
		break; //DR
	case 5:
		r++;
		c--;
		break; //DL
		//RIGHT, LEFT
	case 6:
		c++;
		break; //R
	case 7:
		c--;
		break; //L
	}
	//If off board, return -1
	if (r < 0 || r > 7 || c < 0 || c > 7) return -1;

	//Return position
	return r * 8 + c;
}
//----------------------------------------------------------------------------------------------------------
void CopyBoardToArchive(uchar archiveX, uchar board[], uchar pType) {

	//Store passed board[64] and ptype into archive

	int posX = 0;
	uchar oppRow;
	uchar compRow;
	int colBit;
	for (int rowX = 0; rowX < 8; rowX++) {
		oppRow = 0;
		compRow = 0;
		colBit = 0x80;
		while (colBit != 0) {
			if (board[posX] == eptOpp) {
				oppRow |= (uchar) colBit;
			} else if (board[posX] == eptComp) {
				compRow |= (uchar) colBit;
			}
			colBit >>= 1;
			posX++;
		}
		archivedBoards[archiveX].oppRows[rowX] = oppRow;
		archivedBoards[archiveX].compRows[rowX] = compRow;
	}
	//Finally, store player
	archivedBoards[archiveX].nextPlayer = pType;

}
//----------------------------------------------------------------------------------------------------------
uchar LoadBoardFromArchive(uchar archiveX, uchar board[]) {

	//Store passed board[64] and ptype into packedBoard

	int posX = 0;
	uchar oppRow;
	uchar compRow;
	int colBit;
	for (int rowX = 0; rowX < 8; rowX++) {
		oppRow = archivedBoards[archiveX].oppRows[rowX];
		compRow = archivedBoards[archiveX].compRows[rowX];
		colBit = 0x80;
		while (colBit != 0) {
			if ((colBit & oppRow) != 0) {
				board[posX] = eptOpp;
			} else if ((colBit & compRow) != 0) {
				board[posX] = eptComp;
			} else {
				board[posX] = eptNone;
			}
			colBit >>= 1;
			posX++;
		}
	}
	return archivedBoards[archiveX].nextPlayer;
}

