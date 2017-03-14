/* Implementation file for LEDMatrix Logic
 *
 *
 *
 *
 */
 #include "shiftreg.c"
 #include <bit.h>

// LED Matrix Global Variables
unsigned char board[8][8] = { { 0 } };
unsigned char Level = 1;
unsigned char LED[8] = {0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
unsigned char blue[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
unsigned char green[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
//unsigned char red[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Struct to simplify indexing of positions in board
typedef struct _coordinate {
	unsigned char xpos;
	unsigned char ypos;
} coordinate;

coordinate PaddlePos[3];
coordinate BallPos;

// Generates bricks according to lvl number
// Lvl 1 is basic, top three rows are bricks
// Lvl 2 is fancier, intersperse bricks separated by 2 spaces for top 4 rows
// Lvl 3 is a boss battle, top row is boss (4-wide), next 3 rows are basic bricks
void GenerateLevel(unsigned char lvlNum) {
	int i;
	int j;
	if (lvlNum == 1) {
		for (i = 0; i < 3; i++) {
			for (j = 0; j < 8; j++) {
				board[i][j] = 'G';
			}
		}
	} else if (lvlNum == 2) {

	} else if (lvlNum == 3) {

	}
}

void ClearRow(unsigned char rowNum) {
	for (int i = 0; i < 8; i++) {
		board[rowNum][i] = '0';
	}
}


// Initiates paddle object on matrix
void GeneratePaddle(){
	int i;
	int j = 0;

	// Clear paddle from bottom row of board
	ClearRow(7);

	// Center Paddle
	for (i = 3; i <= 5; i++) {
		PaddlePos[j].xpos = i;
		PaddlePos[j].ypos = 7;
		board[7][i] = 'P';
		j++;
	}
}

// Writes paddle position onto board
void WritePaddle() {
	for (int i = 0; i < 3; i++) {
		board[PaddlePos[i].ypos][PaddlePos[i].xpos] = 'P';
	}
}


// Initiates ball object on matrix
void GenerateBall() {
	// Clear ball position on the screen(?)

	// Generate ball at starting position
	BallPos.xpos = 6;
	BallPos.ypos = 4;
	board[6][4] = 'A';
}


/*
// Causes ball to bounce at 45 deg angle relative to its previous position
void Bounce(unsigned char bounceFlag, unsigned char xPos, unsigned char yPos) {

}*/


// Generates boss object for level 3 of the game
void GenerateBoss(coordinate BossPos) {

}


void ReadGreen() {
	
	int i, j;
	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++) {
			if (board[i][j] == 'G' || board[i][j] == 'A')
				green[i] &= SetBit(green[i], j, 0);
			else
				green[i] |= SetBit(green[i], j, 1);
		}
	}
}

void ReadBlue() {
	int i, j;
	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++) {
			if (board[i][j] == 'P' || board[i][j] == 'S')
				blue[i] &= SetBit(blue[i], j, 0);
			else
				blue[i] |= SetBit(blue[i], j, 1);
		}
	}
}

/*
// ====================
// SM1: DEMO LED matrix
// ====================
enum SM1_States {sm1_display};
int SM1_Tick(int state) {
	// === Local Variables ===
	static unsigned char column_val = 0x01; // sets the pattern displayed on columns
	static unsigned char column_sel = 0x7F; // grounds column to display pattern
	// === Transitions ===
	switch (state) {
		case sm1_display:
		break;
		default:
		state = sm1_display;
		break;
	}
	// === Actions ===
	switch (state) {
		case sm1_display:   // If illuminated LED in bottom right corner
		if (column_sel == 0xFE && column_val == 0x80) {
			column_sel = 0x7F; // display far left column
			column_val = 0x01; // pattern illuminates top row
		}
		// else if far right column was last to display (grounded)
		else if (column_sel == 0xFE) {
			column_sel = 0x7F; // resets display column to far left column
			column_val = column_val << 1; // shift down illuminated LED one row
		}
		// else Shift displayed column one to the right
		else {
			column_sel = (column_sel >> 1) | 0x80;
		}
		break;
		default:
		break;
	}
	LED = column_val; // LED holds column pattern
	green = 0xFF;
	blue = column_sel; // blue selects column to display blue pattern

	transmit_3_data(green, LED, blue);

	return state;
};

enum SM2_States {sm2_display};
int SM2_Tick(int state) {
	// === Local Variables ===
	static unsigned char column_val = 0x01; // sets the pattern displayed on columns
	static unsigned char column_sel = 0x7F; // grounds column to display pattern
	// === Transitions ===
	switch (state) {
		case sm2_display:
		break;
		default:
		state = sm2_display;
		break;
	}
	// === Actions ===
	switch (state) {
		case sm2_display:   // If illuminated LED in bottom right corner
		if (column_sel == 0xFE && column_val == 0x80) {
			column_sel = 0x7F; // display far left column
			column_val = 0x01; // pattern illuminates top row
		}
		// else if far right column was last to display (grounded)
		else if (column_sel == 0xFE) {
			column_sel = 0x7F; // resets display column to far left column
			column_val = column_val << 1; // shift down illuminated LED one row
		}
		// else Shift displayed column one to the right
		else {
			column_sel = (column_sel >> 1) | 0x80;
		}
		break;
		default:
		break;
	}
	LED = column_val; // LED holds column pattern
	blue = 0xFF;
	green = column_sel; // green selects column to display green pattern

	transmit_3_data(green, LED, blue);

	return state;
};*/