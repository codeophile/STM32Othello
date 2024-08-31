/********************************************************************************
 OTH_Main.h
 ********************************************************************************/

#ifndef INC_OTH_MAIN_H_
#define INC_OTH_MAIN_H_

#include "WS64_Cmd.h"

enum PlayerType {
	//These are stored in the board array to indicate both the player that owns
	//the position and the color shown in the position.
	eptNone = ecoOff,
	eptComp = ecoBlu,
	eptOpp= ecoRed,
	//Used to indicate tie on status LEDs and to flash flips
	eptNoneYel = ecoYel,
};

enum StatusLEDX{
	//Order that LEDs are wired
	estatGameOverX = 0,
	estatPlayerX = 1,
	estatMustPassX = 2,
};

enum GameState {
	egsOppsTurnPrep,
	egsOppsTurn,
	egsOppsFlipsBeingShown,
	egsOppMustPass,
	egsCompsTurnPrep,
	egsCompsTurn,
	egsCompsFlipsBeingShown,
	egsCompMustPass,
	egsGameOverPrep,
	egsGameOver,
	egsDelay,

	egsDrawTest,
	egsWaitForDraw,
	egsSetMode,
};

//From AI: The maximum number of possible plays at any given moment occurs when there are
//exactly 32 empty squares on the board, and all the remaining squares are occupied by pieces
//of alternating colors (one player's pieces are flanked by the opponent's pieces in all directions).
#define PLAY_LIST_SIZE 32

//From me and AI: Max possible is when playing a corner and flipping 6 in each direction: horiz, vert, diagonal
#define FLIP_LIST_SIZE 18

//ARCHIVED BOARDS are stored for undoing plays all the way back to the beginning if desired.
//Storing occurs when either side makes a play.
//If player undos, he can redo back to where he was. BUT, once a new play is made after undo,
//board storage is reset to that play and redo stops there.
//I chose to save up to 80 boards for 64 possbile plays plus up to 16 passes.
//FROM AI: While there is no theoretical maximum number of times a player can pass, it is
//generally uncommon for a player to pass more than a few times in a single game, as players
//typically have legal moves available until the endgame stages.
#define ARCHIVED_BOARD_COUNT 80

typedef struct  {
	uchar oppRows[8];
	uchar compRows[8];
	uchar nextPlayer;
}ArchivedBoard;

//From OTH_Primitives.c
void StoreFlipList(uchar board[], int posX, uchar pType);
void StorePlayableList(uchar board[], uchar pType);
void CopyBoardToArchive(uchar archiveX, uchar board[], uchar pType);
uchar LoadBoardFromArchive(uchar archiveX, uchar board[]);
bool IsPlayable(uchar board[], int posX, uchar pType);

#endif /* INC_OTH_MAIN_H_ */
