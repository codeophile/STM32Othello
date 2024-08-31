/********************************************************************************
 OTH_GetMove.c
 ********************************************************************************/

#include "APP.h"
#include "OTH_Main.h"
#include <stdlib.h> //for getting random number

#define MAX_SCORE_INDEX 10

//From OTH_Primitives
extern uchar playableList[PLAY_LIST_SIZE];
extern int playableListCOUNT;
extern uchar flipList[FLIP_LIST_SIZE];
extern int flipListCOUNT;

//From OTH_Main
extern uchar boardNow[64];

//From this file
schar CFNewlyAdded(int stdPosX);
schar CENewlyPlayable(uchar pType, int stdPosX);
schar CENewlyAdded(int stdPosX);
void StoreScoreForTrialPos(int trialPosX, schar score[]);
int GetCompMove(); //as copied from C#

//SCORING TRIALS
//These arrays represent positions on board.
uchar boardTrial[64];
bool oppPlayableNow[64];
bool compPlayableNow[64];
bool oppPlayableInTrial[64];
bool compPlayableInTrial[64];
bool addedAndFlippedToCompInTrial[64];

//The playableToConsider[] array mirrors the playableList[]
//It indicates indexes in playableList[] that are under consideration for the next move.
//A value <=0 indicates false, i.e., to be avoided. >0 indicates true, i.e., to consider.
//This was done instead of using bool for the C# version:
// - The <=0 value indicates the negative of scoreX that set it false to make it easily accessable to the debug string.
//The microcontroller version doesn't use this info, but I just leave it in for easy copy from C#
//CAN BE NEGATIVE!
schar playableToConsider[PLAY_LIST_SIZE];

//ScoreData[index in playableList][score item]
//ScoreData values are small numbers. I think the max would be the number of flips which won't exceed 64
//CAN BE NEGATIVE!
schar ScoreData[PLAY_LIST_SIZE][MAX_SCORE_INDEX];

//STANDARD VIEW INTO QUADRANTS
//0.3.8.F
//1.2.7.E
//4.5.6.D
//9.A.B.C
uchar AbsPosFromQuadrantPos[4][16] = {
	{ 0, 8, 9, 1, 16, 17, 18, 10, 2, 24, 25, 26, 27, 19, 11, 3 },
	{ 7, 6, 14, 15, 5, 13, 21, 22, 23, 4, 12, 20, 28, 29, 30, 31 },
	{ 63, 55, 54, 62, 47, 46, 45, 53, 61, 39, 38, 37, 36, 44, 52, 60 },
	{ 56, 57, 49, 48, 58, 50, 42, 41, 40, 59, 51, 43, 35, 34, 33, 32 },
};
//----------------------------------------------------------------------------------------------------------
int GetCompMoveByMode(bool modeEasy) {

	//Caller has verified that list count isn't zero

	if (modeEasy) {
		//Get random number from 0 to playableListCOUNT-1
		//(note that remainder can never = count since largest remainder after division is count-1)
		int playX = rand() % (playableListCOUNT);
		return playableList[playX];
	} else {
		return GetCompMove();
	}
}

//----------------------------------------------------------------------------------------------------------
int GetCompMove() {

	//---------------------------------------
	//Initialize
	//---------------------------------------

	int posX;
	int playX;

	//NOTE: Caller has already verified that list count isn't zero

	//If only one move can be made, make it.
	if (playableListCOUNT == 1) {
		posX = playableList[0];
		return posX;
	}

	//Store computer playable and opponent playable indexed by position
	for (posX = 0; posX < 64; posX++) {
		oppPlayableNow[posX] = IsPlayable(boardNow, posX, eptOpp);
		//Clear previous values. Will set just below.
		compPlayableNow[posX] = false;
	}
	for (playX = 0; playX < playableListCOUNT; playX++) {
		posX = playableList[playX];
		compPlayableNow[posX] = true;
	}

	//---------------------------------------
	//Calculate score items for each possible play.
	//---------------------------------------

	for (playX = 0; playX < playableListCOUNT; playX++) {
		posX = playableList[playX];
		StoreScoreForTrialPos(posX, ScoreData[playX]);
	}

	//TEMPORARY
	//WriteScores();

	//---------------------------------------
	//COMPARE PLAYS BY SCORE, ONE SCORE ITEM AT A TIME.
	//Note that the way scoring works, when one is negative another can never be positive and vice versa.
	//If only one play left, stop checking and play it
	//If all are zero, move on to next score item(probably most common).
	//If any are > 0, stop checking and play highest (or first if match)
	//If all are <0 (could be down to one item), stop checking and play highest(or first if match)
	//If any are <0, remove them from list and continue to next score item
	//---------------------------------------

	//The playableToConsider[] array mirrors the playableList[]
	//Initialize list to true (see notes in declaration above)
	for (int cnt = 0; cnt < playableListCOUNT; cnt++)
		playableToConsider[cnt] = 1;

	//Compare scores
	int score;
	int scoreX;
	int maxScore = 0;
	int maxScorePlayX;
	int scoreForDebugString;

	//Precheck items
	int playableToConsiderCount;
	int playsGT0Count;
	int playsEQ0Count;
	int playsLT0Count;

	//Order of scoring is important since priorty is given to the earliest score where previous scores are all equal.
	for (scoreX = 0; scoreX < 11; scoreX++) {
		playableToConsiderCount = 0;
		playsGT0Count = 0;
		playsEQ0Count = 0;
		playsLT0Count = 0;
		//Precheck
		maxScorePlayX = -1;
		maxScore = -99;
		//---------------------------------------
		//Get stats on this score item
		//---------------------------------------
		for (playX = 0; playX < playableListCOUNT; playX++) {
			//var xxx = ScoreData[playListX];
			if (playableToConsider[playX] > 0) {
				playableToConsiderCount++;
				score = ScoreData[playX][scoreX];
				if (score > 0) playsGT0Count++;
				if (score < 0) playsLT0Count++;
				if (score == 0) playsEQ0Count++;
				if (score > maxScore) {
					//If all are equal, max score will be first play
					maxScore = score;
					maxScorePlayX = playX;
				}
			}
		}

		//Should never arrive here with less than 2 options to consider
		if (playableToConsiderCount < 2) {
			//An error occured. Return a valid play just to avoid a crash (C# version displays this error)
			return playableList[0];
		}

		//If all are zero, move on to next score item (probably most common).
		if (playsEQ0Count == playableToConsiderCount) continue;

		//If any plays are positive, play highest
		if (playsGT0Count > 0) {
			posX = playableList[maxScorePlayX];
			scoreForDebugString = ScoreData[maxScorePlayX][scoreX];
			goto DoExit;
		}
		//At this point, all plays are <= 0
		//If all are <0, i.e, none are zero, stop checking and play highest
		//LATER: If two are higher than remaining, could remove the lower ones and continue
		if (playsLT0Count == playableToConsiderCount) {
			posX = playableList[maxScorePlayX];
			scoreForDebugString = ScoreData[maxScorePlayX][scoreX];
			goto DoExit;
		}
		//At this point, at least one play is 0 and at least one play < 0
		//From this point on, stop considering the plays that are <0
		for (playX = 0; playX < playableListCOUNT; playX++) {
			if (playableToConsider[playX] > 0) {
				score = ScoreData[playX][scoreX];
				if (score < 0) {
					//Set to false in a way that stores scoreX for debug string
					playableToConsider[playX] = (schar) (-scoreX);
					playableToConsiderCount--;
				}
			}
		}
		//At this point the plays with negative scores have been removed from consideration, leaving only those with zero scores.
		//If only one item remains, play it
		if (playableToConsiderCount == 1) {
			for (playX = 0; playX < playableListCOUNT; playX++) {
				if (playableToConsider[playX] > 0) {
					posX = playableList[playX];
					scoreForDebugString = ScoreData[playX][scoreX];
					goto DoExit;
				}
			}
		}

	}        //end for loop each scoreX

	//An error occured, so eturn a valid play just to avoid a crash (C# version displays this error)
	return playableList[0];

	DoExit:

	//Branching to this point is useful in the C# version to apply some debugging code
	//that is deleted in the microcontroller version which just leaves this branch in place.
	//UNCOMMENT BELOW in microcontroller version to avoid "set but not used" warning
	score = scoreForDebugString;
	//Return the move
	return posX;

}
//----------------------------------------------------------------------------------------------------------
void StoreScoreForTrialPos(int trialPosX, schar score[]) {

	//---------------------------------------
	//CREATE TRIAL BOARD WHICH IS CURRENT BOARD AFTER TRIAL PLAY IS MADE
	//---------------------------------------

	//Initialize
	for (int cnt = 0; cnt < 64; cnt++) {
		boardTrial[cnt] = boardNow[cnt];
		addedAndFlippedToCompInTrial[cnt] = false;
	}

	//Store move and flips in trial board
	addedAndFlippedToCompInTrial[trialPosX] = true;
	boardTrial[trialPosX] = eptComp;
	StoreFlipList(boardNow, trialPosX, eptComp);
	int flipCount = flipListCOUNT;
	for (int flipX = 0; flipX < flipCount; flipX++) {
		int posX = flipList[flipX];
		addedAndFlippedToCompInTrial[posX] = true;
		boardTrial[posX] = eptComp;
	}

	//---------------------------------------
	//STORE PLAYABLE MOVES IN TRIAL BOARD
	//---------------------------------------

	for (int posX = 0; posX < 64; posX++) {
		compPlayableInTrial[posX] = IsPlayable(boardTrial, posX, eptComp);
		oppPlayableInTrial[posX] = IsPlayable(boardTrial, posX, eptOpp);
	}

	//---------------------------------------
	//SCORE THIS MOVE; CF=Corner Full; CE = Corner Empty
	//Order of scoring is important since priorty is given to the earliest score where previous scores are all equal.
	//---------------------------------------

	//Score[0] Good if play is a corner
	if (trialPosX == 0 || trialPosX == 7 || trialPosX == 63 || trialPosX == 56) score[0] = 1;
	else score[0] = 0;

	//ALL SCORING BELOW CONSIDERS EACH QUADRANT
	//0.3.8.F
	//1.2.7.E
	//4.5.6.D
	//9.A.B.C

	//Bad if opponent can newly play corner
	score[1] = 0;
	score[1] -= CENewlyPlayable(eptOpp, 0);
	//Good if computer can newly play corner
	score[2] = 0;
	score[2] += CENewlyPlayable(eptComp, 0);
	//Bad if computer adds any of 1-3
	score[3] = 0;
	for (int cnt = 1; cnt <= 3; cnt++) {
		score[3] -= CENewlyAdded(cnt);
	}
	//Good if opponent can newly play 1-3
	score[4] = 0;
	for (int cnt = 1; cnt <= 3; cnt++) {
		score[4] += CENewlyPlayable(eptOpp, cnt);
	}
	//Good if computer adds any of 4-8
	score[5] = 0;
	for (int cnt = 4; cnt <= 8; cnt++) {
		score[5] += CENewlyAdded(cnt);
	}
	//Bad if opponent can newly play 4-8
	score[6] = 0;
	for (int cnt = 4; cnt <= 8; cnt++) {
		score[6] -= CENewlyPlayable(eptOpp, cnt);
	}
	//Good if computer can newly play 4-8
	score[7] = 0;
	for (int cnt = 4; cnt <= 8; cnt++) {
		score[7] += CENewlyPlayable(eptComp, cnt);
	}
	//Bad if computer adds any of 9-15
	score[8] = 0;
	for (int cnt = 9; cnt <= 15; cnt++) {
		score[8] -= CENewlyAdded(cnt);
	}
	//I saw it base move here when 1 corner was still empty (so all 4 corners don't need to be full)
	//NOW CONSIDERING PLAYS WHERE CORNER IS FULL
	//Good if computer adds edges 1,3,4,8,9,15
	score[9] = 0;
	score[9] += CFNewlyAdded(1);
	score[9] += CFNewlyAdded(3);
	score[9] += CFNewlyAdded(4);
	score[9] += CFNewlyAdded(8);
	score[9] += CFNewlyAdded(9);
	score[9] += CFNewlyAdded(15);
	//Last comparison. Just use number of flips
	//I haven't yet seen a move based on this
	score[10] = (schar) flipListCOUNT;
	//IMPORTANT: Don't exceed MAX_SCORE_INDEX

}
//----------------------------------------------------------------------------------------------------------
schar CFNewlyAdded(int stdPosX) {

	//Only computer moves are newly added
	//CF=Corner Full; adds points only if corner is full by comp or opp

	//Corner Full
	//compAddedTrial
	schar count = 0;
	for (int quadX = 0; quadX < 4; quadX++) {
		int absCornerX = AbsPosFromQuadrantPos[quadX][0];
		//If corner full... (could have used boardTrial too since this functions value will be ignored in special case that trial played the corner)
		if (boardNow[absCornerX] != eptNone) {
			//Increment counter if trial added (through move and flips)
			int absPosX = AbsPosFromQuadrantPos[quadX][stdPosX];
			if (addedAndFlippedToCompInTrial[absPosX]) count++;
		}
	}
	return count;

}
//----------------------------------------------------------------------------------------------------------
schar CENewlyAdded(int stdPosX) {

	//Only computer moves are newly added
	//CE=Corner Empty; adds points only if corner is empty

	schar count = 0;
	for (int quadX = 0; quadX < 4; quadX++) {
		int absCornerX = AbsPosFromQuadrantPos[quadX][0];
		//If corner empty (could have used boardTrial too since these counts won't even be considered when trial played the corner)
		if (boardNow[absCornerX] == eptNone) {
			//Increment counter if trial added (through moves and flips)
			int absPosX = AbsPosFromQuadrantPos[quadX][stdPosX];
			if (addedAndFlippedToCompInTrial[absPosX]) count++;
		}
	}
	return count;

}
//----------------------------------------------------------------------------------------------------------
schar CENewlyPlayable(uchar pType, int stdPosX) {

	//CE=Corner Empty
	//For each quadrant that has an empty corner, adds 1 count if position is newly playable by player type

	schar count = 0;
	for (int quadX = 0; quadX < 4; quadX++) {
		int absCornerX = AbsPosFromQuadrantPos[quadX][0];
		//If corner empty (could have used boardTrial too since these counts won't even be considered when trial played the corner)
		if (boardNow[absCornerX] == eptNone) {
			int absPosX = AbsPosFromQuadrantPos[quadX][stdPosX];
			//Increment counter if move has NEWLY become playable by trial
			if (pType == eptComp) {
				if (compPlayableInTrial[absPosX] && !compPlayableNow[absPosX]) count++;
			} else {
				if (oppPlayableInTrial[absPosX] && !oppPlayableNow[absPosX]) count++;
			}
		}
	}
	return count;

}
