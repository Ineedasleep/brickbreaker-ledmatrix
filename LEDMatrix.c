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
unsigned char currentLevel = 1;
signed char Bosslife = 0; // Can dip into negative
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
coordinate BossPos[4];
coordinate BallPos;
coordinate BallPrev;

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
		board[0][7] = 'G'; board[0][6] = 'G'; board[0][3] = 'G'; board[0][2] = 'G';
		board[1][5] = 'G'; board[1][4] = 'G'; board[1][1] = 'G'; board[1][0] = 'G';
		board[2][7] = 'G'; board[2][6] = 'G'; board[2][3] = 'G'; board[2][2] = 'G';
		board[3][5] = 'G'; board[3][4] = 'G'; board[3][1] = 'G'; board[3][0] = 'G';
	} else if (lvlNum == 3) {
		for (i = 0; i < 8; i++) {
			board[1][i] = 'G';
		}
		for (i = 0; i < 8; i++) {
			board[3][i] = 'G';
		}
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

void UpdateBall() {
	board[BallPos.ypos][BallPos.xpos] = 'A';
	board[BallPrev.ypos][BallPrev.xpos] = '0';
}

void UpdatePrev() {
	BallPrev.ypos = BallPos.ypos;
	BallPrev.xpos = BallPos.xpos;
}

// Initiates ball object on matrix
void GenerateBall() {
	// Clear ball position on the screen
	if (board[BallPos.ypos][BallPos.xpos] != 'G' && board[BallPos.ypos][BallPos.xpos] != 'P')
		board[BallPos.ypos][BallPos.xpos] = '0';

	// Generate ball at starting position
	BallPos.xpos = 4;
	BallPos.ypos = 6;
	board[6][4] = 'A';
}

// Breaks a single (2x1) brick
void Break(unsigned char xpos, unsigned char ypos) {
	board[ypos][xpos] = '0';
	if (xpos == 0 || xpos == 2 || xpos == 4 || xpos == 6) {
		board[ypos][xpos+1] = '0';
	} else if (xpos == 1 || xpos == 3 || xpos == 5 || xpos == 7) {
		board[ypos][xpos-1] = '0';
	}
}

// Generates boss object for level 3 of the game
void GenerateBoss() {
	Bosslife = 3;
	int i;
	for (i = 0; i <= 3; i++) {
		BossPos[i].xpos = i+1;
		BossPos[i].ypos = 0;
		board[0][i+1] = 'S';
	}
}


void ReadGreen() {
	
	int i, j;
	for (i = 0; i < 8; i++) {
		for (j = 0; j < 8; j++) {
			if (board[i][j] == 'G' || board[i][j] == 'A') // Brick or ball
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
			if (board[i][j] == 'P' || board[i][j] == 'S') // Paddle or boss
				blue[i] &= SetBit(blue[i], j, 0);
			else
				blue[i] |= SetBit(blue[i], j, 1);
		}
	}
}

unsigned char CheckWin() {
	int i, j;
	for (i = 0; i < 4; i++) {
		for (j = 0; j < 8; j++) {
			if (board[i][j] == 'G' || Bosslife != 0)
				return 0;
		}
	}
	return 1;
}

unsigned char CheckBossWin() {
	if (Bosslife == 0)
		return 1;
	else
		return CheckWin();
}