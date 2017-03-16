/*
 * BrickBreaker.c
 *
 * Created: 3/9/2017 12:59:49 PM
 * Author : Ethan Valdez
 */ 

#include <avr/io.h>
#include <avr/eeprom.h>
#include <timer.h>
#include <scheduler.h>
#include <keypad.h>
#include "LEDMatrix.c"
#include "LCDScreen.c"

/*
 * State Machines:
 * Input -- TODO: Add joystick readings
 * Paddle
 * Level
 * Ball/Impact -- TODO: implement scorekeeping
 * Boss -- TODO: Write everything
 * LEDDisplay
 * LCDDisplay -- TODO: Fix bugs
 */

void ADC_init() {
	ADCSRA |= (1 << ADEN) | (1 << ADSC) | (1 << ADATE);
	// ADEN: setting this bit enables analog-to-digital conversion.
	// ADSC: setting this bit starts the first conversion.
	// ADATE: setting this bit enables auto-triggering. Since we are
	//        in Free Running Mode, a new conversion will trigger whenever
	//        the previous conversion completes.
}

// Global variables
unsigned char Left = 0;		// Left button input, shared between Input and Paddle
unsigned char Right = 0;	// Right button input, shared between Input and Paddle
unsigned char P = 0;		// Pause/Start button input, shared between Input, Paddle, and Ball
unsigned char numLives = 3;		// Number of lives the user has left
uint16_t score;				// The score (will increment by 10 for each brick broken)
uint16_t highscore;
unsigned char updateScore;	// Flag to indicate that score has been updated (reprint)
unsigned char LevelWon = 0;	// Flag to indicate that a level was completed
unsigned char LevelLost = 0;// Flag to indicate that a level was lost
unsigned char GameOver = 0; // Flag to indicate that game was lost (numLives == 0)
unsigned char GameWon = 0;	// Flag to indicate that the boss was defeated

/*------------------------- TASK 1: INPUT -------------------------*/
enum Input_states {Input_Start, Input_Wait, Input_Left, Input_Right, 
					Input_Pause1, Input_Pause2} input_state;
int Input_Tick(int state) {
	// Local variables
	unsigned char keyin = GetKeypadKey();
	//unsigned char joyclick = PINA & 0x02;
	unsigned short joy1 = ADC;

	// === Transitions ===
	switch(state) {
		case Input_Start:
			Left = 0;
			Right = 0;
			P = 1;
			if (keyin == 'A' /*|| joyclick*/)
				state = Input_Wait;
			else
				state = Input_Start;
			break;
		case Input_Wait:
			if (keyin == 'A' /*|| joyclick*/)
				state = Input_Pause1;
			else if (keyin == '3' || joy1 >= 750)
				state = Input_Left;
			else if (keyin == '2' || joy1 <= 250)
				state = Input_Right;
			else
				state = Input_Wait;
			break;
		case Input_Left:
			if (keyin == '3' || joy1 >= 750)
				state = Input_Left;
			else
				state = Input_Wait;
			break;
		case Input_Right:
			if (keyin == '2' || joy1 <= 200)
				state = Input_Right;
			else
				state = Input_Wait;
			break;
		case Input_Pause1:
			if (keyin == 'A' /*|| joyclick*/)
				state = Input_Pause1;
			else
				state = Input_Pause2;
		case Input_Pause2:
			if (keyin == 'A' /*|| joyclick*/)
				state = Input_Wait;
			else
				state = Input_Pause2;
			break;
		default:
			state = Input_Start;
			break;
	}

	// === Actions ===
	switch(state) {
		case Input_Start:
			Left = 0;
			Right = 0;
			P = 1;
			break;
		case Input_Wait:
			P = 0;
			Left = 0;
			Right = 0;
			break;
		case Input_Left:
			Left = 1;
			Right = 0;
			P = 0;
			break;
		case Input_Right:
			Left = 0;
			Right = 1;
			P = 0;
			break;
		case Input_Pause1:
			Left = 0;
			Right = 0;
			P = 1;
			break;
		case Input_Pause2:
			P = 1;
			break;
		default:
			break;
	}

	return state;
};


/*------------------------- TASK 2: PADDLE -------------------------*/
enum Paddle_states {Paddle_Start, Paddle_Wait, Paddle_Left, Paddle_LeftWait,
					 Paddle_Right, Paddle_RightWait, Paddle_Pause, Paddle_Lost,
					 Paddle_Win, Paddle_Complete, Paddle_GameOver} paddle_state;
int Paddle_Tick(int state) {
	// Local variables
	int i;
	static unsigned char leftCount = 0;
	static unsigned char rightCount = 0;
	static unsigned char lostCount = 0;
	static unsigned char winCount = 0;

	// === Transitions ===
	switch(state) {
		case Paddle_Start:
			GeneratePaddle();
			if (!P)
				state = Paddle_Wait;
			else
				state = Paddle_Start;
			break;
		case Paddle_Wait:
			if (P)
				state = Paddle_Pause;
			else if (LevelLost)
				state = Paddle_Lost;
			else if (LevelWon)
				state = Paddle_Win;
			else if (Left && !Right && !P)
				state = Paddle_Left;
			else if (Right && !Left && !P)
				state = Paddle_Right;
			else
				state = Paddle_Wait;
			break;
		case Paddle_Left:
			state = Paddle_LeftWait;
			break;
		case Paddle_LeftWait:
			if (P)
				state = Paddle_Pause;
			else if (Left && !Right && !P) {
				leftCount++;
				if (leftCount < 3)
					state = Paddle_LeftWait;
				else
					state = Paddle_Left;
			} else if (Right && !Left && !P)
				state = Paddle_Right;
			else
				state = Paddle_Wait;
			break;
		case Paddle_Right:
			state = Paddle_RightWait;
			break;
		case Paddle_RightWait:
			if (P)
				state = Paddle_Pause;
			else if (Left && !Right && !P)
				state = Paddle_Left;
			else if (Right && !Left && !P) {
				rightCount++;
				if (rightCount < 3)
					state = Paddle_RightWait;
				else
					state = Paddle_Right;
			 } else
				state = Paddle_Wait;
			break;
		case Paddle_Pause:
			if (P)
				state = Paddle_Pause;
			else if (Left && !Right && !P)
				state = Paddle_Left;
			else if (Right && !Left && !P)
				state = Paddle_Right;
			else
				state = Paddle_Wait;
			break;
		case Paddle_Lost:
			if (lostCount >= 100) {	// 50 * 100 = 5000ms
				lostCount = 0;
				GeneratePaddle();
				LevelLost = 0;
				state = Paddle_Wait;
			} else {
				lostCount++;
				state = Paddle_Lost;
			}
			break;
		case Paddle_Win:
			if (GameWon)
				state = Paddle_Complete;
			else if (!GameWon && winCount >= 100) { // 50 * 100 = 5000ms
				winCount = 0;
				GeneratePaddle();
				LevelWon = 0;
				state = Paddle_Wait;
			} else {
				winCount++;
				state = Paddle_Win;
			}
			break;
		case Paddle_Complete:
			if (winCount >= 100) {
				winCount = 0;
				GeneratePaddle();
				LevelWon = 0;
				GameWon = 0;
				currentLevel = 1;
				P = 1;
				state = Paddle_Start;
			} else {
				winCount++;
				state = Paddle_Complete;
			}
			break;
		case Paddle_GameOver:
			if (lostCount >= 100) {	// 50 * 100 = 5000ms
				lostCount = 0;
				GeneratePaddle();
				LevelLost = 0;
				P = 1;
				state = Paddle_Start;
			} else {
				lostCount++;
				state = Paddle_Lost;
			}
			break;
		default:
			state = Paddle_Start;
			break;
	}

	// === Actions ===
	switch(state) {
		case Paddle_Start:
			break;
		case Paddle_Wait:
			break;
		case Paddle_Left:
			leftCount = 0;
			if (PaddlePos[2].xpos < 7) {
				ClearRow(7);
				for (i = 0; i < 3; i++) {
					PaddlePos[i].xpos++;
				}
				WritePaddle();
			}
			break;
		case Paddle_LeftWait:
			break;
		case Paddle_Right:
			rightCount = 0;
			if (PaddlePos[0].xpos > 0) {
				ClearRow(7);
				for (i = 0; i < 3; i++) {
					PaddlePos[i].xpos--;
				}
				WritePaddle();
			}
			break;
		case Paddle_RightWait:
			break;
		case Paddle_Pause:
			break;
		case Paddle_Lost:
			break;
		case Paddle_Win:
			break;
		case Paddle_Complete:
			break;
		case Paddle_GameOver:
			break;
		default:
			break;
	}

	return state;
};


/*------------------------- TASK 3: LEVEL -------------------------*/
enum Level_states {Level_Start, Level_Generate, Level_Play, Level_Complete, Level_Lost, 
					Level_GameOver, Level_Congrats} level_state;
int Level_Tick(int state) {
	// Local variables
	static unsigned short count = 0;

	// === Transitions ===
	switch(state) {
		case Level_Start:
			state = Level_Generate;
			break;
		case Level_Generate:
			state = Level_Play;
			break;
		case Level_Play:
			if (LevelWon) {
				if (currentLevel < 4) currentLevel++;
				state = Level_Complete;
			} else if (LevelLost)
				state = Level_Lost;
			else
				state = Level_Play;
			break;
		case Level_Complete:
			if (currentLevel == 4)
				state = Level_Congrats;
			else if (count >= 102) { // 50 * 100 = 5000
				count = 0;
				state = Level_Generate;
			} else {
				count++;
				state = Level_Complete;
			}
			break;
		case Level_Lost:
			if (GameOver)
				state = Level_GameOver;
			else if (count >= 100) {	// 50 * 100 = 5000
				count = 0;
				state = Level_Play;
			} else {
				count++;
				state = Level_Lost;
			}
			break;
		case Level_GameOver:
			if (count >= 200) { // 50 * 200 = 10000ms
				count = 0;
				numLives = 3;
				currentLevel = 1;
				state = Level_Start;
			} else {
				count++;
				state = Level_GameOver;
			}
			break;
		case Level_Congrats:
			if (count >= 200) { // 50 * 200 = 10000ms
				count = 0;
				state = Level_Start;
			} else {
				count++;
				state = Level_Congrats;
			}
			break;
		default:
			state = Level_Start;
			break;
	}

	// === Actions ===
	switch(state) {
		case Level_Start:
			break;
		case Level_Generate:
			if (currentLevel <= 3) GenerateLevel(currentLevel);
			break;
		case Level_Play:
			break;
		case Level_Complete:
			break;
		case Level_Lost:
			break;
		case Level_GameOver:
			break;
		case Level_Congrats:
			break;
		default:
			break;
	}
	return state;
};

/*------------------------- TASK 4: BOSS -------------------------*/
enum Boss_states {Boss_Standby, Boss_Start, Boss_Left, Boss_Right, Boss_Pause,
					Boss_Complete, Boss_LostLife, Boss_Gameover} boss_state;
int Boss_Tick(int state) {
	// Local Variables
	unsigned char count = 0;
	int i;
	static enum Boss_states prevState;

	// === Transitions ===
	switch(state) {
		case Boss_Standby:
			if (currentLevel != 3)
				state = Boss_Standby;
			else
				state = Boss_Start;
			break;
		case Boss_Start:
			state = Boss_Left;
			break;
		case Boss_Left:
			if (GameWon)
				state = Boss_Complete;
			else if (P) {
				state = Boss_Pause;
				prevState = Boss_Left;
			} else if (BossPos[3].xpos < 7) {
				state = Boss_Left;
			} else {
				state = Boss_Right;
			}
			break;
		case Boss_Right:
			if (GameWon)
				state = Boss_Complete;
			else if (P) {
				state = Boss_Pause;
				prevState = Boss_Right;
			} else if (BossPos[0].xpos > 0) {
				state = Boss_Right;
			} else {
				state = Boss_Left;
			}
			break;
		case Boss_Complete:
			if (count < 40) {
				count++;
				state = Boss_Complete;
			} else {
				count = 0;
				state = Boss_Standby;
			}
			break;
		case Boss_LostLife:
			if (count < 20) {
				count++;
				state = Boss_LostLife;
			} else {
				count = 0;
				state = Boss_Start;
			}
			break;
		case Boss_Gameover:
			if (count < 40) {
				count++;
				state = Boss_Gameover;
			} else {
				count = 0;
				Bosslife = 0;
				state = Boss_Standby;
			}
			break;
		case Boss_Pause:
			if (!P)
				state = prevState;
			else
				state = Boss_Pause;
		default:
			break;
	}

	// === Actions ===
	switch(state) {
		case Boss_Standby:
			break;
		case Boss_Start:
			GenerateBoss();
			break;
		case Boss_Left:
			ClearRow(0);
			for (i = 0; i < 4; i++) {
				BossPos[i].xpos++;
			}
			for (i = 0; i < 4; i++) {
				board[0][BossPos[i].xpos] = 'S';
			}
			break;
		case Boss_Right:
			ClearRow(0);
			for (i = 0; i < 4; i++) {
				BossPos[i].xpos--;
			}
			for (i = 0; i < 4; i++) {
				board[0][BossPos[i].xpos] = 'S';
			}
			break;
		case Boss_Complete:
			break;
		case Boss_LostLife:
			break;
		case Boss_Gameover:
			break;
		case Boss_Pause:
			break;
		default:
			break;
	}
	return state;
}

/*------------------------- TASK 5: BALL -------------------------*/
enum Ball_states {Ball_Start, Ball_UpRight, Ball_UpLeft, Ball_DownRight, Ball_DownLeft, 
					Ball_Death, Ball_DeathWait, Ball_Pause1, Ball_Pause2, Ball_Win,
					Ball_GameOver, Ball_Complete} ball_state;
int Ball_Tick(int state) {
	// Local variables
	static unsigned short loseCount = 0;
	static unsigned short winCount = 0;
	static enum Ball_states PreviousState = 0;

	// === Transitions ===
	switch(state) {
		case Ball_Start:
			GenerateBall();
			if (P)
				state = Ball_Start;
			else
				state = Ball_UpRight;	// Ball starts moving up and right when games starts
			break;
		case Ball_UpRight:
			if (P) {
				state = Ball_Pause1;
				PreviousState = Ball_UpRight;
			}
			// Bounce logic
			else if (BallPos.xpos == 0 && board[BallPos.ypos-1][BallPos.xpos+1] == 'G') { // Bounce off brick, right wall
				Break(BallPos.xpos+1, BallPos.ypos-1);
				score += 10;
				updateScore = 1;
				if (CheckWin() == 1) {
					LevelWon = 1;
					state = Ball_Win;
				} else
					state = Ball_DownLeft;
			} else if (BallPos.xpos == 0 && board[BallPos.ypos-1][BallPos.xpos+1] == 'S') { // Bounce off boss, right wall
				Bosslife--;
				score += 50;
				updateScore = 1;
				if (CheckBossWin() == 1) {
					LevelWon = 1;
					GameWon = 1;
					state = Ball_Win;
				} else
					state = Ball_DownLeft; 
			} else if (BallPos.xpos != 0 && BallPos.ypos == 1
						&& board[BallPos.ypos-1][BallPos.xpos-1] == 'S') { // Bounce off boss
				Bosslife--;
				score += 50;
				updateScore = 1;
				if (CheckBossWin() == 1) {
					LevelWon = 1;
					GameWon = 1;
					state = Ball_Win;
				} else
					state = Ball_DownRight;
			} else if (BallPos.xpos == 0 && BallPos.ypos != 0) { // Bounce off right wall
				state = Ball_UpLeft;
			} else if (BallPos.xpos != 0 && BallPos.ypos == 0 
						&& board[BallPos.ypos+1][BallPos.xpos-1] != 'G') { // Bounce off top wall, no brick
				state = Ball_DownRight;
			} else if (BallPos.xpos != 0 && BallPos.ypos == 0
						&& board[BallPos.ypos+1][BallPos.xpos-1] == 'G') {
				Break(BallPos.xpos-1,BallPos.ypos+1);
				score += 10;
				updateScore = 1;
				if (CheckWin() == 1) {
					LevelWon = 1;
					state = Ball_Win;
				} else
					state = Ball_DownLeft;
			} else if (BallPos.xpos == 0 && BallPos.ypos == 0) { // Bounce off right corner
				state = Ball_DownLeft;
			} else if (board[BallPos.ypos-1][BallPos.xpos-1] == 'G'
						&& board[BallPos.ypos+1][BallPos.xpos-1] != 'G') {	// Bounce off a brick, not right wall
				Break(BallPos.xpos-1, BallPos.ypos-1);
				score += 10;
				updateScore = 1;
				if (CheckWin() == 1) { 
					LevelWon = 1;
					state = Ball_Win;
				} else
					state = Ball_DownRight;
			} else if (board[BallPos.ypos-1][BallPos.xpos-1] == 'G' 
						&& board[BallPos.ypos+1][BallPos.xpos-1] == 'G') { // Bounce off brick, brick in way
				Break(BallPos.xpos-1, BallPos.ypos+1);
				Break(BallPos.xpos-1, BallPos.ypos-1);
				score += 20;
				updateScore = 1;
				if (CheckWin() == 1) {
					LevelWon = 1;
					state = Ball_Win;
				} else
					state = Ball_DownLeft;
			} else {	// Keep traveling in current direction
				state = Ball_UpRight;
			}
			break;
		case Ball_UpLeft:
			if (P) {
				state = Ball_Pause1;
				PreviousState = Ball_UpLeft;
			}
			// Bounce logic
			else if (BallPos.xpos == 7 && board[BallPos.ypos-1][BallPos.xpos-1] == 'G') { // Bounce off brick, left wall
				Break(BallPos.xpos-1, BallPos.ypos-1);
				score += 10;
				updateScore = 1;
				if (CheckWin() == 1) {
					LevelWon = 1;
					state = Ball_Win;
				} else
					state = Ball_DownRight;
			} else if (BallPos.xpos == 7 && board[BallPos.ypos-1][BallPos.xpos-1] == 'S') { // Bounce off boss, left wall
				Bosslife--;
				score += 50;
				updateScore = 1;
				if (CheckBossWin() == 1) {
					LevelWon = 1;
					GameWon = 1;
					state = Ball_Win;
				} else
					state = Ball_DownRight; 
			} else if (BallPos.xpos != 7 && BallPos.ypos == 1
						&& board[BallPos.ypos-1][BallPos.xpos+1] == 'S') { // Bounce off boss
				Bosslife--;
				score += 50;
				updateScore = 1;
				if (CheckBossWin() == 1) {
					LevelWon = 1;
					GameWon = 1;
					state = Ball_Win;
				} else
					state = Ball_DownLeft;
			} else if (BallPos.xpos == 7 && BallPos.ypos != 0) { // Bounce off left wall
				state = Ball_UpRight;
			} else if (BallPos.xpos != 7 && BallPos.ypos == 0
						&& board[BallPos.ypos+1][BallPos.xpos+1] == 'G') { // Bounce off brick
				Break(BallPos.xpos+1,BallPos.ypos+1);
				score += 10;
				updateScore = 1;
				if (CheckWin() == 1) {
					LevelWon = 1;
					state = Ball_Win;
				} else
					state = Ball_DownRight;
			} else if (BallPos.xpos != 7 && BallPos.ypos == 0) { // Bounce off top wall
				state = Ball_DownLeft;
			} else if (BallPos.xpos == 7 && BallPos.ypos == 0) { // Bounce off left corner
				state = Ball_DownRight;
			} else if (board[BallPos.ypos-1][BallPos.xpos+1] == 'G'
						&& board[BallPos.ypos+1][BallPos.xpos+1] != 'G') {	// Bounce off a brick, not left wall
				Break(BallPos.xpos+1, BallPos.ypos-1);
				score += 10;
				updateScore = 1;
				if (CheckWin() == 1) {
					LevelWon = 1;
					state = Ball_Win;
				} else 
					state = Ball_DownLeft;
			} else if (board[BallPos.ypos+1][BallPos.xpos+1] == 'G' 
						&& board[BallPos.ypos-1][BallPos.xpos+1] == 'G') { // Bounce off brick, brick in way
				Break(BallPos.xpos+1, BallPos.ypos+1);
				Break(BallPos.xpos+1, BallPos.ypos-1);
				score += 20;
				updateScore = 1;
				if (CheckWin() == 1) {
					LevelWon = 1;
					state = Ball_Win;
				} else
					state = Ball_DownRight;
			} else {	// Keep traveling in current direction
				state = Ball_UpLeft;
			}
			break;
		case Ball_DownRight:
			if (P) {
				state = Ball_Pause1;
				PreviousState = Ball_DownRight;
			}
			// Bounce logic
			else if (BallPos.ypos == 7) { // Ball passed the paddle
				state = Ball_Death;
			} else if (BallPos.xpos == 0 && BallPos.ypos == 6
						&& board[BallPos.ypos+1][BallPos.xpos+1] == 'P') {	// Corner bounce with paddle
				state = Ball_UpLeft;
			} else if (BallPos.xpos == 0 && BallPos.ypos != 7) { // Bounce off right wall
				state = Ball_DownLeft;
			} else if (board[BallPos.ypos+1][BallPos.xpos-1] == 'P') { // Bounce off paddle
				state = Ball_UpRight;
			} else if (board[BallPos.ypos+1][BallPos.xpos-1] == 'G' 
						&& board[BallPos.ypos-1][BallPos.xpos-1] != 'G') { // Bounce off brick, no brick in way
				Break(BallPos.xpos-1, BallPos.ypos-1);
				score += 10;
				updateScore = 1;
				if (CheckWin() == 1) {
					LevelWon = 1;
					state = Ball_Win;
				} else 
					state = Ball_UpRight;
			} else if (board[BallPos.ypos+1][BallPos.xpos-1] == 'G' 
						&& board[BallPos.ypos-1][BallPos.xpos-1] == 'G') { // Bounce off brick, brick in way
				Break(BallPos.xpos-1, BallPos.ypos+1);
				Break(BallPos.xpos-1, BallPos.ypos-1);
				score += 20;
				updateScore = 1;
				if (CheckWin() == 1) {
					LevelWon = 1;
					state = Ball_Win;
				} else
					state = Ball_DownRight;
			} else {	// Keep traveling in current direction
				state = Ball_DownRight;
			}
			break;
		case Ball_DownLeft:
			if (P) {
				state = Ball_Pause1;
				PreviousState = Ball_DownLeft;
			}
			// Bounce logic
			else if (BallPos.ypos == 7) { // Ball passed the paddle
				state = Ball_Death;
			} else if (BallPos.xpos == 7 && BallPos.ypos == 6
						&& board[BallPos.ypos+1][BallPos.xpos-1] == 'P') {	// Corner bounce with paddle
				state = Ball_UpRight;
			} else if (BallPos.xpos == 7 && BallPos.ypos != 7) { // Bounce off left wall
				state = Ball_DownRight;
			} else if (board[BallPos.ypos+1][BallPos.xpos+1] == 'P') { // Bounce off paddle
				state = Ball_UpLeft;
			} else if (board[BallPos.ypos+1][BallPos.xpos+1] == 'G' 
						&& board[BallPos.ypos-1][BallPos.xpos+1] != 'G') { // Bounce off brick, no brick in way
				Break(BallPos.xpos+1, BallPos.ypos-1);
				score += 10;
				updateScore = 1;
				if (CheckWin() == 1) LevelWon = 1;
				state = Ball_UpLeft;
			} else if (board[BallPos.ypos+1][BallPos.xpos+1] == 'G' 
						&& board[BallPos.ypos-1][BallPos.xpos+1] == 'G') { // Bounce off brick, brick in way
				Break(BallPos.xpos+1, BallPos.ypos+1);
				Break(BallPos.xpos+1, BallPos.ypos-1);
				score += 20;
				updateScore = 1;
				if (CheckWin() == 1) LevelWon = 1;
				state = Ball_DownLeft;
			} else {	// Keep traveling in current direction
				state = Ball_DownLeft;
			}
			break;
		case Ball_Death:
			state = Ball_DeathWait;
			break;
		case Ball_DeathWait:
			if (P) {
				PreviousState = Ball_Death;
				state = Ball_Pause1;
			}
			else if (GameOver)
				state = Ball_GameOver;
			else if (loseCount < 20) { // 250 * 20 = 5000ms
				loseCount++;
				state = Ball_DeathWait;
			}
			else {
				loseCount = 0;
				state = Ball_Start;
			}
			break;
		case Ball_Pause1:
			state = Ball_Pause2;
			break;
		case Ball_Pause2:
			if (P) {
				state = Ball_Pause2;
			} else {
				state = PreviousState;
			}
			break;
		case Ball_Win:
			if (GameWon)
				state = Ball_Complete;
			else if (!GameWon && winCount < 20) {	// 250 * 20 = 5000ms
				winCount++;
				state = Ball_Win;
			} else {
				winCount = 0;
				LevelWon = 0;
				state = Ball_Start;
			}
			break;
		case Ball_GameOver:
			if (loseCount < 40) {	// 250 * 40 = 10000ms
				loseCount++;
				state = Ball_GameOver;
			} else {
				loseCount = 0;
				GameOver = 0;
				LevelLost = 0;
				state = Ball_Start;
			}
			break;
		case Ball_Complete:
			if (winCount < 40) {
				winCount++;
				state = Ball_Complete;
			} else {
				winCount = 0;
				LevelWon = 0;
				GameWon = 0;
				P = 1;
				state = Ball_Start;
			}
			break;
		default:
			state = Ball_Start;
			break;
	}

	// === Actions ===
	switch(state) {
		case Ball_Start:
			break;
		case Ball_UpRight:
			UpdatePrev();
			BallPos.xpos--;
			BallPos.ypos--;
			UpdateBall();
			break;
		case Ball_UpLeft:
			UpdatePrev();
			BallPos.xpos++;
			BallPos.ypos--;
			UpdateBall();
			break;
		case Ball_DownRight:
			UpdatePrev();
			BallPos.xpos--;
			BallPos.ypos++;
			UpdateBall();
			break;
		case Ball_DownLeft:
			UpdatePrev();
			BallPos.xpos++;
			BallPos.ypos++;
			UpdateBall();
			break;
		case Ball_Death:
			if (numLives > 0) {
				numLives--;
				LevelLost = 1;
			}
			if (numLives == 0) {
				GameOver = 1;
			}
			break;
		case Ball_DeathWait:
			break;
		case Ball_Pause1:
			if (PreviousState == Ball_UpRight) {
				BallPos.xpos++;
				BallPos.ypos++;
			} else if (PreviousState == Ball_UpLeft) {
				BallPos.xpos--;
				BallPos.ypos++;
			} else if (PreviousState == Ball_DownRight) {
				BallPos.xpos++;
				BallPos.ypos--;
			} else if (PreviousState == Ball_DownLeft) {
				BallPos.xpos--;
				BallPos.ypos--;
			}
			break;
		case Ball_Pause2:
			break;
		case Ball_Win:
			break;
		case Ball_GameOver:
			break;
		case Ball_Complete:
			break;
		default:
			break;
	}
	return state;
}

/*------------------------- TASK 6: LED DISPLAY-------------------------*/
enum LEDDisplay_states {LEDDisp_Start, LEDDisp_Display} LEDdisplay_state;
int LEDDisplay_Tick(int state) {
	// Local variables
	int i;
	//int j = 0;

	// === Transitions ===
	switch(state) {
		case LEDDisp_Start:
			state = LEDDisp_Display;
			break;
		case LEDDisp_Display:
			state = LEDDisp_Display;
			break;
	}

	// === Actions ===
	switch(state) {
		case LEDDisp_Start:
			break;
		case LEDDisp_Display:
			ReadGreen();
			ReadBlue();
			for (i = 0; i < 8; i++) {
				transmit_3_data(green[i], LED[i], blue[i]);
			}
			break;
	}
	return state;
};


// Dependent on: GameOver, GameWon, LevelWon, LevelLost, score, numLives, updateScore
/*------------------------- TASK 7: LCD DISPLAY -------------------------*/
enum LCD_Displaystates {LCD_Start, LCD_Wait, LCD_Play, LCD_Pause, LCD_UpdateScore, LCD_UpdateLife, 
						LCD_WinLevel, LCD_LoseLevel, LCD_WinGame, LCD_LoseGame} LCD_Displaystate;
int LCDDisplay_Tick(int state) {
	// Local variables
	static unsigned short winCount = 0;
	static unsigned short loseCount = 0;

	// === Transitions ===
	switch(state) {
		case LCD_Start:
			highscore = eeprom_read_word((uint16_t*) 46);
			LCD_DisplayString(1, "BrickBreaker");
			LCD_AppendString(17, "High Score: ");
			LCD_WriteData('0' + ((highscore % 1000) / 100));
			LCD_WriteData('0' + ((highscore % 100) / 10));
			LCD_WriteData('0' + (highscore % 10));
			if (P)
				state = LCD_Start;
			else
				state = LCD_Play;
			break;
		case LCD_Wait:
			if (GameOver)
				state = LCD_LoseGame;
			else if (GameWon)
				state = LCD_WinGame;
			else if (LevelWon)
				state = LCD_WinLevel;
			else if (LevelLost)
				state = LCD_UpdateLife;
			else if (updateScore)
				state = LCD_UpdateScore;
			else if (P)
				state = LCD_Pause;
			else
				state = LCD_Wait;
			break;
		case LCD_Play:
			state = LCD_Wait;
			break;
		case LCD_Pause:
			if (P) 
				state = LCD_Pause;
			else
				state = LCD_Play;
			break;
		case LCD_UpdateScore:
			updateScore = 0;
			state = LCD_Wait;
			break;
		case LCD_UpdateLife:
			state = LCD_LoseLevel;
			break;
		case LCD_WinLevel:
			if (currentLevel == 4)
				state = LCD_WinGame;
			else if (LevelWon)
				state = LCD_WinLevel;
			else
				state = LCD_Play;
			break;
		case LCD_LoseLevel:
			if (LevelLost)
				state = LCD_LoseLevel;
			else
				state = LCD_Play;
			break;
		case LCD_WinGame:
			if (winCount <= 19) { // 500 * 20 = 10000ms
				winCount++;
				state = LCD_WinGame;
			} else {
				winCount = 0;
				if (score > highscore) {
					eeprom_update_word((uint16_t*) 46, score);					
				}
				score = 0;
				state = LCD_Start;
			}
			break;
		case LCD_LoseGame:
			if (loseCount <= 19) { // 500 * 20 = 10000ms
				loseCount++;
				state = LCD_LoseGame;
			} else {
				loseCount = 0;
				if (score > highscore) {
					eeprom_update_word((uint16_t*) 46, score);
				}
				for (int i = 0; i < 4; i++) {
					ClearRow(i);
				}
				score = 0;
				state = LCD_Start;
			}
			break;
		default:
			state = LCD_Start;
			break;
	}

	// === Actions ===
	switch(state) {
		case LCD_Start:
			break;
		case LCD_Wait:
			break;
		case LCD_Play:
			LCD_DisplayString(1, "Lives: ");
			LCD_WriteData('0' + numLives);
			LCD_AppendString(17, "Score: ");
			LCD_WriteData('0' + ((score % 1000) / 100));
			LCD_WriteData('0' + ((score % 100) / 10));
			LCD_WriteData('0' + (score % 10));
			break;
		case LCD_Pause:
			LCD_DisplayString(1, "PAUSED");
			break;
		case LCD_UpdateScore:
			LCD_Cursor(24);
			LCD_WriteData('0' + ((score % 1000) / 100));
			LCD_WriteData('0' + ((score % 100) / 10));
			LCD_WriteData('0' + (score % 10));
			break;
		case LCD_UpdateLife:
			LCD_Cursor(8);
			LCD_WriteData('0' + numLives);
			break;
		case LCD_WinLevel:
			LCD_DisplayString(1, "Level Complete!");
			break;
		case LCD_LoseLevel:
			LCD_DisplayString(1, "Lost a life!");
			break;
		case LCD_WinGame:
			LCD_DisplayString(1, "Congratulations! You Won!");
			break;
		case LCD_LoseGame:
			LCD_DisplayString(1, "GAME OVER");
			break;
		default:
			break;
	}
	return state;
}

int main(void)
{
    DDRA = 0xF0; PORTA = 0x0F;	// A0-3 are joystick input, A4-7 are shift reg output
	//DDRA = 0xFF; PORTA = 0x00;	// For Debugging
    DDRB = 0xFF; PORTB = 0x00;	// B0-1 are control lines for LCD display
	DDRC = 0xF0; PORTC = 0x0F;	// Keypad input
	DDRD = 0xFF; PORTD = 0X00;	// Data lines for LCD display

	/*uint16_t Word;
	Word = 50;
	eeprom_update_word((uint16_t*) 46, Word);*/

    const unsigned short numTasks = 7;
    const unsigned short GCDPeriod = 25;
    task tasks[numTasks];
    unsigned char i = 0;

    // Task 1: Input
    tasks[i].state = 0;
    tasks[i].period = 25;
    tasks[i].elapsedTime = 0;
    tasks[i].TickFct = &Input_Tick;
	i++;

	// Task 2: Paddle
	tasks[i].state = 0;
	tasks[i].period = 50;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &Paddle_Tick;
	i++;

	// Task 3: Level
	tasks[i].state = 0;
	tasks[i].period = 50;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &Level_Tick;
	i++;

	// Task 4: Boss
	tasks[i].state = 0;
	tasks[i].period = 250;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &Boss_Tick;
	i++;

	// Task 5: Ball
	tasks[i].state = 0;
	tasks[i].period = 250;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &Ball_Tick;
	i++;

	// Task 6: LEDDisplay
	tasks[i].state = 0;
	tasks[i].period = 25;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &LEDDisplay_Tick;
	i++;

	// Task 7: LCDDisplay
	tasks[i].state = 0;
	tasks[i].period = 500;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &LCDDisplay_Tick;

    TimerSet(GCDPeriod);
    TimerOn();
	LCD_init();
	ADC_init();

    while (1) {
	    // Scheduler code
	    for ( i = 0; i < numTasks; i++ ) {
		    // Task is ready to tick
		    if ( tasks[i].elapsedTime == tasks[i].period ) {
			    // Setting next state for task
			    tasks[i].state = tasks[i].TickFct(tasks[i].state);
			    // Reset the elapsed time for next tick.
			    tasks[i].elapsedTime = 0;
		    }
		    tasks[i].elapsedTime += GCDPeriod;
	    }
	    while(!TimerFlag);
	    TimerFlag = 0;
    }
}

