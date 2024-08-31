/********************************************************************************
DIN.h
********************************************************************************/


#ifndef INC_DIN_H_
#define INC_DIN_H_

void DIN_Update();

enum btn {
	ebtnUndoPlay = 0x01,
	ebtnRedoPlay = 0x02,
	ebtnSetMode = 0x04,
	ebtnNewGame = 0x08,
	ebtnMakePlay = 0x10,
	ebtnSelectMove = 0x20,
	ebtnShowScore = 0x40,
	ebtnPass = 0x80,
	ebtnSetModeReleased = 0x104,
};

#endif /* INC_DIN_H_ */
