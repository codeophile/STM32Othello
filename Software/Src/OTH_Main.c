/********************************************************************************
 OTH_Main.c

 ********************************************************************************/

#include "App.h"
#include "WS64_Cmd.h"
#include "OTH_Main.h"
#include "DIN.h"

//In this file
void RedoPlay();
void UndoPlay();
void ArchiveBoardForUndo(uchar pType);
void ResetUndoRedoStatus();
bool IsBoardFull();
void ApplyFlipListToBoardNow(uchar pType);
void UpdateStatus(uchar newStatusX);
void UpdateScore();
void DisplayStatus();
void DisplayBoardNow();
void InitNewGame(bool startGame);
void ApplyGameMode();

//Outside this file
void SLED_DisplayStatus(uchar colorX[]);
int GetCompMoveByMode(bool modeEasy);

extern uint DIN_buttonLatch;
extern bool WS64CmdListCompleteFlag;

//From OTH_Primitives
extern uchar playableList[PLAY_LIST_SIZE];
extern int playableListCOUNT;
extern uchar flipList[FLIP_LIST_SIZE];
extern int flipListCOUNT;

//Mode
#define MAX_GAME_MODE 2
uchar gameMode;
uchar gameModeTentative;
bool modeAutoSelectOppsFirstTentativePlay;
bool modeEasy;

//BOARD POSITIONS ARE INDEXED FROM 0-63
//Row 0 is on top, from left to right are positions 0-7, next row down are positions 8-15, etc.
uchar boardNow[64];
uchar statusLEDs[3];

//Status data
int gState;
int tentativeOppPlay;
int compPlay;
uchar oppScore;
uchar compScore;
bool oppJustPassed;
bool compJustPassed;
uchar currentLeader;
uchar currentPlayer;

//Showing moves/flips
bool flipShowSequentially = false; //vs flash
int flipShowSeqX;
int flashCount;

//Flash support
bool flashOn;

//Delay support
int delayTics;
//Used for delay, set mode, show score
int stateAfter;

//Archiving boards for undo/redo
int archiveBoardX;
int archiveBoardCount;
bool undoOccured;

//----------------------------------------------------------------------------------------------------------
void MainInit() {

	//For testing
//	statusLEDs[estatGameOverX] = ecoRed;
//	statusLEDs[estatPlayerX] = ecoBlu;
//	statusLEDs[estatMustPassX] = ecoYel;
//	SLED_DisplayStatus(statusLEDs);

	WS64_Init();
	InitNewGame(false);
	ApplyGameMode();

	//---------------------------
	//TEMP FOR TEST
	//WS64_DisplayChr(18);

	//--------------------

	//Do startup show first
	WS64_CmdStartShow();
	WS64_CmdListStart(boardNow);
	stateAfter = egsOppsTurnPrep;
	gState = egsWaitForDraw;

}

//----------------------------------------------------------------------------------------------------------
void MainUpdate() {

	int posX;
	flashOn = !flashOn;

	if (gState != egsWaitForDraw && gState != egsSetMode) {
		if (ebtnNewGame & DIN_buttonLatch) {
			InitNewGame(true);
		}
		if (ebtnUndoPlay & DIN_buttonLatch) {
			UndoPlay();
		} else if (ebtnRedoPlay & DIN_buttonLatch) {
			RedoPlay();
		}
	}

	switch (gState) {

	case egsOppsTurnPrep:
		//Transient state - here for one pass
		currentPlayer = eptOpp;
		UpdateStatus(estatPlayerX);
		//Store board for undo; will return here on redo.
		ArchiveBoardForUndo(eptOpp);
		oppJustPassed = false;
		StorePlayableList(boardNow, eptOpp);
		if (playableListCOUNT > 0) {
			tentativeOppPlay = -1;
			gState = egsOppsTurn;
		} else {
			//Can't move. Could be due to board full or can't flip
			if (IsBoardFull()) {
				gState = egsGameOverPrep;
			} else {
				if (compJustPassed) {
					//Game over once neither player can move
					gState = egsGameOverPrep;
				} else {
					UpdateStatus(estatMustPassX);
					gState = egsOppMustPass;
				}
			}
		}
		break;

	case egsOppMustPass:
		//Waits for button press
		if (ebtnPass & DIN_buttonLatch) {
			oppJustPassed = true;
			gState = egsCompsTurnPrep;
		}
		break;

	case egsOppsTurn:
		//Only get here if player has at least one move.
		//Remains here until player presses play.
		//Setting tentative play here instead of in oppsTurnPrep allow checking from off to on to work here
		if (tentativeOppPlay == -1 && modeAutoSelectOppsFirstTentativePlay) {
			tentativeOppPlay = 0;
		}
		if (ebtnShowScore & DIN_buttonLatch) {
			WS64_CmdListShowScore(compScore, oppScore);
			WS64_CmdListStart(boardNow);
			stateAfter = egsOppsTurn;
			gState = egsWaitForDraw;
			break;
		} else if (ebtnSetMode & DIN_buttonLatch) {
			gameModeTentative = gameMode;
			WS64_DisplayChr(gameModeTentative + 16);
			stateAfter = egsOppsTurn;
			gState = egsSetMode;
			break;
		} else if (ebtnSelectMove & DIN_buttonLatch) {
			//Set next tentative play
			if (++tentativeOppPlay >= playableListCOUNT) tentativeOppPlay = 0;
		} else if (ebtnMakePlay & DIN_buttonLatch) {
			//If a tentative play has not yet been selected, pressing play does nothing.
			if (tentativeOppPlay >= 0) {
				posX = playableList[tentativeOppPlay];
				boardNow[posX] = eptOpp;
				//Make play determines flip but doesn't make them yet
				DisplayBoardNow();
				//Init for displaying flips
				//statusLED_Player = eptNone;
				StoreFlipList(boardNow, posX, eptOpp);
				ResetUndoRedoStatus();
				//Init for showing flips
				flipShowSeqX = 0;
				flashCount = 0;
				gState = egsOppsFlipsBeingShown;
				break;
			}
		}
		//Flash tentativePLay
		if (tentativeOppPlay >= 0 && flashOn) {
			posX = playableList[tentativeOppPlay];
			boardNow[posX] = eptOpp;
			DisplayBoardNow();
			boardNow[posX] = eptNone;
		} else {
			DisplayBoardNow();
		}
		break;

	case egsOppsFlipsBeingShown:
		if (flipShowSequentially) {
			//Sequentially show flips
			if (flipShowSeqX < flipListCOUNT) {
				posX = flipList[flipShowSeqX++];
				boardNow[posX] = eptOpp;
				DisplayBoardNow();
			} else {
				//Done showing flips. Get comps next move
				gState = egsCompsTurnPrep;
			}
			break;
		} else {
			//Flash positions to be flipped
			if (++flashCount == 8) {
				ApplyFlipListToBoardNow(eptOpp);
				gState = egsCompsTurnPrep;
			} else if (flashCount % 2 == 1) {
				//odd number; set flips off
				ApplyFlipListToBoardNow(eptNoneYel);
			} else {
				//even number; set flips previous owner
				ApplyFlipListToBoardNow(eptComp);
			}
			DisplayBoardNow();
			break;
		}

	case egsCompsTurnPrep:
		//Transient state - here for one pass
		currentPlayer = eptComp;
		UpdateStatus(estatPlayerX);
		//Store board for undo; will return here on redo.
		ArchiveBoardForUndo(eptComp);
		compJustPassed = false;
		StorePlayableList(boardNow, eptComp);
		if (playableListCOUNT > 0) {
			flashCount = 0;
			compPlay = GetCompMoveByMode(modeEasy);
			delayTics = 3;
			stateAfter = egsCompsTurn;
			gState = egsDelay;
			//gState = egsCompsTurn;
		} else {
			//Can't move. Could be due to board full or can't flip
			if (IsBoardFull()) {
				gState = egsGameOverPrep;
			} else {
				if (oppJustPassed) {
					//Game over once neither player can move
					gState = egsGameOverPrep;
				} else {
					UpdateStatus(estatMustPassX);
					gState = egsCompMustPass;
				}
			}
		}
		break;

	case egsCompMustPass:
		//Wait for button press
		if (ebtnPass & DIN_buttonLatch) {
			gState = egsOppsTurnPrep;
		}
		break;

	case egsCompsTurn:
		//Wait for button press
		if (ebtnShowScore & DIN_buttonLatch) {
			WS64_CmdListShowScore(compScore, oppScore);
			WS64_CmdListStart(boardNow);
			stateAfter = egsCompsTurn;
			gState = egsWaitForDraw;
			break;
		} else if (ebtnSetMode & DIN_buttonLatch) {
			gameModeTentative = gameMode;
			WS64_DisplayChr(gameModeTentative + 16);
			stateAfter = egsCompsTurn;
			gState = egsSetMode;
			break;
		} else if (ebtnMakePlay & DIN_buttonLatch) {
			//playableList was stored in compsTurnPrep above and length checked as >0
			StoreFlipList(boardNow, compPlay, eptComp);
			ResetUndoRedoStatus();
			boardNow[compPlay] = eptComp;
			DisplayBoardNow();
			flashCount = 0;
			gState = egsCompsFlipsBeingShown;
		} else {
			//Flash comps move
			if (flashOn) {
				boardNow[compPlay] = eptComp;
			} else {
				boardNow[compPlay] = eptNone;
			}
			DisplayBoardNow();
		}
		break;

	case egsCompsFlipsBeingShown:
		if (flipShowSequentially) {
			//Showing flips sequentially
			if (flipShowSeqX < flipListCOUNT) {
				posX = flipList[flipShowSeqX++];
				boardNow[posX] = eptComp;
				DisplayBoardNow();
				break;
			}
		} else {
			//Flash positions to be flipped
			if (++flashCount == 8) {
				ApplyFlipListToBoardNow(eptComp);
				DisplayBoardNow();
			} else if (flashCount % 2 == 1) {
				//odd number; set flips off
				ApplyFlipListToBoardNow(eptNoneYel);
				DisplayBoardNow();
				break;
			} else {
				//even number; set flips previous owner
				ApplyFlipListToBoardNow(eptOpp);
				DisplayBoardNow();
				break;
			}
		}
		//Showing complete, proceed
		if (modeAutoSelectOppsFirstTentativePlay) {
			//Short delay after displaying comps play before flashing opps first choice
			delayTics = 3;
			stateAfter = egsOppsTurnPrep;
			gState = egsDelay;
		} else {
			gState = egsOppsTurnPrep;
		}
		break;

	case egsGameOverPrep:
		currentPlayer = eptNone;
		UpdateStatus(estatGameOverX);
		gState = egsGameOver;
		break;

	case egsGameOver:
		if (ebtnShowScore & DIN_buttonLatch) {
			WS64_CmdListShowScore(compScore, oppScore);
			WS64_CmdListStart(boardNow);
			stateAfter = egsGameOver;
			gState = egsWaitForDraw;
		}
		break;

	case egsDelay:
		if (--delayTics <= 0) gState = stateAfter;
		break;

	case egsWaitForDraw:
		if (ebtnNewGame & DIN_buttonLatch) {
			WS64_CmdListCancel();
			InitNewGame(true);
		}
		if (WS64CmdListCompleteFlag) {
			WS64CmdListCompleteFlag = false;
			gState = stateAfter;
			//To speed showing of flashing move
			flashOn = false;
		}
		break;

	case egsSetMode:
		if (ebtnSelectMove & DIN_buttonLatch) {
			if (++gameModeTentative > MAX_GAME_MODE) gameModeTentative = 0;
			WS64_DisplayChr(gameModeTentative + 16);
		} else if (ebtnMakePlay & DIN_buttonLatch) {
			//Save and apply mode
			DisplayBoardNow();
			gameMode = gameModeTentative;
			ApplyGameMode();
			gState = stateAfter;
		} else if (ebtnSetMode & DIN_buttonLatch) {
			//Exit without save
			DisplayBoardNow();
			gState = stateAfter;
		}
		break;

//	case egsDrawTest:
//		//Set to this state at boot for WS64 testing
//		WS64_CmdListTest();
//		//WS64_CmdListShowScore(9, 23);
//		WS64_CmdListStart(0);
//		stateAfter = egsDrawTest; //keep looping
//		gState = egsWaitForDraw;
//		break;

	} //switch

	//Clear latched button presses
	DIN_buttonLatch = 0;

}
//----------------------------------------------------------------------------------------------------------
void InitNewGame(bool startGame) {

	//Clear board
	for (int posX = 0; posX < 64; posX++) {
		boardNow[posX] = eptNone;
	}

	boardNow[27] = eptComp;
	boardNow[28] = eptOpp;
	boardNow[35] = eptOpp;
	boardNow[36] = eptComp;

	oppJustPassed = false;
	compJustPassed = false;

	//Init storing for undo/redo
	archiveBoardX = -1;
	archiveBoardCount = 0;

	//Status LEDs (player and leader will be updated in game state)
	statusLEDs[estatGameOverX] = eptNone;
	statusLEDs[estatMustPassX] = eptNone;

	//Opponent goes first
	if (startGame) {
		gState = egsOppsTurnPrep;
	}

}
//----------------------------------------------------------------------------------------------------------
void ApplyGameMode(){

	switch(gameMode){

	case 0:
		modeAutoSelectOppsFirstTentativePlay = true;
		modeEasy=true;
		break;

	case 1:
		modeAutoSelectOppsFirstTentativePlay = true;
		modeEasy=false;
		break;

	case 2:
		modeAutoSelectOppsFirstTentativePlay = false;
		modeEasy=false;
		break;

	}
}
//----------------------------------------------------------------------------------------------------------
void DisplayBoardNow() {

	WS64_Display(boardNow);
}
//----------------------------------------------------------------------------------------------------------
void DisplayStatus() {

	void SLED_DisplayStatus(uchar colorX[]);

	SLED_DisplayStatus(statusLEDs);

}
//----------------------------------------------------------------------------------------------------------
void UpdateScore() {

	//Get score
	oppScore = 0;
	compScore = 0;
	for (int posX = 0; posX < 64; posX++) {
		uchar owner = boardNow[posX];
		if (owner == eptOpp) {
			oppScore++;
		} else if (owner == eptComp) {
			compScore++;
		}
	}
	//Determine leader;
	if (oppScore > compScore) {
		currentLeader = eptOpp;
	} else if (compScore > oppScore) {
		currentLeader = eptComp;
	} else {
		//Tie
		currentLeader = eptNoneYel;
	}

}
//----------------------------------------------------------------------------------------------------------
void UpdateStatus(uchar newStatusX) {

	UpdateScore();

	switch (newStatusX) {
	case estatGameOverX:
		statusLEDs[estatGameOverX] = currentLeader;
		statusLEDs[estatPlayerX] = eptNone;
		statusLEDs[estatMustPassX] = eptNone;
		break;
	case estatPlayerX:
		statusLEDs[estatGameOverX] = eptNone;
		statusLEDs[estatPlayerX] = currentPlayer;
		statusLEDs[estatMustPassX] = eptNone;
		break;
	case estatMustPassX:
		statusLEDs[estatGameOverX] = eptNone;
		statusLEDs[estatPlayerX] = currentPlayer;
		statusLEDs[estatMustPassX] = currentPlayer;
		break;
	}

	DisplayStatus();
}
//----------------------------------------------------------------------------------------------------------
void ApplyFlipListToBoardNow(uchar pType) {

	for (int cnt = 0; cnt < flipListCOUNT; cnt++) {
		int posX = flipList[cnt];
		boardNow[posX] = pType;
	}

}
//----------------------------------------------------------------------------------------------------------
bool IsBoardFull() {
	for (int posX = 0; posX < 64; posX++) {
		if (boardNow[posX] == eptNone) return false;
	}
	return true;
}
//----------------------------------------------------------------------------------------------------------
void ResetUndoRedoStatus() {

	undoOccured = false;
	//Should be called when a play is made to handle the situation where player presses undo one or more times
	//Then decides to start playing from an older board. This effectively permanently removes the newer boards.
	archiveBoardCount = archiveBoardX + 1;
}
//----------------------------------------------------------------------------------------------------------
void ArchiveBoardForUndo(uchar pType) {

	//Called on UState.oppsTurnPrep and compsTurnPrep to store board before a new move is made.

	//Don't save any boards once an undo has occured (until a new play occurs)
	//DO NOT INSTEAD USE "if (storedBoardCount != storedBoardCurrentX + 1)" as this will result in extra
	//copy made of most recent board each time an undo then redo is performed.
	if (undoOccured) return;

	archiveBoardX++;
	archiveBoardCount = archiveBoardX + 1;

	//Copy boardNow
	CopyBoardToArchive(archiveBoardX, boardNow, pType);

}

//----------------------------------------------------------------------------------------------------------
void UndoPlay() {

	//Return if we're at start of game
	if (archiveBoardX == 0) return;

	undoOccured = true;

	//Set to previous archive
	archiveBoardX--;

	uchar pType = LoadBoardFromArchive(archiveBoardX, boardNow);

	if (pType == eptOpp) {
		gState = egsOppsTurnPrep;
	} else {
		gState = egsCompsTurnPrep;
	}
	DisplayBoardNow();
}
//----------------------------------------------------------------------------------------------------------
void RedoPlay() {

	//Return if no more archives to return to
	if (archiveBoardCount == archiveBoardX + 1) return;

	//Set to next archive
	archiveBoardX++;

	uchar pType = LoadBoardFromArchive(archiveBoardX, boardNow);

	if (pType == eptOpp) {
		gState = egsOppsTurnPrep;
	} else {
		gState = egsCompsTurnPrep;
	}
	DisplayBoardNow();

}

