/*
 * BrickBreaker.c
 *
 * Created: 3/9/2017 12:59:49 PM
 * Author : Ethan Valdez
 */ 

#include <avr/io.h>
#include <timer.h>
#include <scheduler.h>
#include <keypad.h>
#include "LEDMatrix.c"
#include "LCDScreen.c"

/*
 * State Machines:
 * Input
 * Paddle
 * Level
 * Ball/Impact
 * Display
 */

// Global variables
unsigned char Left = 0;		// Left button input, shared between Input and Paddle
unsigned char Right = 0;	// Right button input, shared between Input and Paddle
unsigned char P = 0;		// Pause/Start button input, shared between Input, Paddle, and Ball
unsigned char numLives;		// Number of lives the user has left
unsigned char score[2];		// The individual digits of the score (will increment by 10 for each brick)
unsigned char LevelWon = 0;	// Flag to indicate that a level was completed
unsigned char LevelLost = 0;// Flag to indicate that a level was lost

/*------------------------- TASK 1: INPUT -------------------------*/
enum Input_states {Input_Start, Input_Wait, Input_Left, Input_Right, 
					Input_Pause1, Input_Pause2} input_state;
int Input_Tick(int state) {
	// Local variables
	unsigned char keyin = GetKeypadKey();
	//unsigned char joyclick = ~PINA & 0x02;
	//unsigned char joy1 = ADC;

	// === Transitions ===
	switch(state) {
		case Input_Start:
			Left = 0;
			Right = 0;
			P = 1;
			if (keyin == 'A')
				state = Input_Wait;
			else
				state = Input_Start;
			break;
		case Input_Wait:
			if (keyin == 'A')
				state = Input_Pause1;
			else if (keyin == '3')
				state = Input_Left;
			else if (keyin == '2')
				state = Input_Right;
			else
				state = Input_Wait;
			break;
		case Input_Left:
			if (keyin == '3')
				state = Input_Left;
			else
				state = Input_Wait;
			break;
		case Input_Right:
			if (keyin == '2')
				state = Input_Right;
			else
				state = Input_Wait;
			break;
		case Input_Pause1:
			if (keyin == 'A')
				state = Input_Pause1;
			else
				state = Input_Pause2;
		case Input_Pause2:
			if (keyin == 'A')
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
					 Paddle_Right, Paddle_RightWait, Paddle_Pause} paddle_state;
int Paddle_Tick(int state) {
	// Local variables
	int i;
	static unsigned char leftCount = 0;
	static unsigned char rightCount = 0;

	// === Transitions ===
	switch(state) {
		case Paddle_Start:	// Also functions as a reset state
			GeneratePaddle();
			if (!P)
				state = Paddle_Wait;
			else
				state = Paddle_Start;
			break;
		case Paddle_Wait:
			if (P)
				state = Paddle_Pause;
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
		default:
			break;
	}

	return state;
};


/*------------------------- TASK 3: LEVEL -------------------------*/
enum Level_states {Level_Start, Level_Generate, Level_Play, Level_Complete, Level_Lost} level_state;
int Level_Tick(int state) {
	// Local variables
	unsigned char count = 0;

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
				if (Level < 3) Level++;
				state = Level_Complete;
			} else if (LevelLost)
				state = Level_Lost;
			else
				state = Level_Play;
			break;
		case Level_Complete:
			if (count >= 4000) {
				count = 0;
				state = Level_Generate;
			} else {
				count++;
				state = Level_Complete;
			}
			break;
		case Level_Lost:
			if (count >= 4000) {
				count = 0;
				state = Level_Start;
			} else {
				count++;
				state = Level_Lost;
			}
			break;
	}

	// === Actions ===
	switch(state) {
		case Level_Start:
			break;
		case Level_Generate:
			GenerateLevel(Level);
			break;
		case Level_Play:
			break;
		case Level_Complete:
			break;
		case Level_Lost:
			break;
	}
	return state;
};


/*------------------------- TASK 4: BALL -------------------------*/


/*------------------------- TASK 5: LED DISPLAY-------------------------*/
enum LEDDisplay_states {LEDDisp_Start, LEDDisp_Display} LEDdisplay_state;
int LEDDisplay_Tick(int state) {
	// Local variables
	int i;
	//int j = 0;

	// === Transitions ===
	switch(state) {
		case LEDDisp_Start:
			LCD_DisplayString(1, "Welcome to BrickBreaker!");	// TODO: Move to LCDDisplay
			numLives = 3;
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
			/*LCD_DisplayString(1, "Lives: ");
			LCD_Cursor(8);
			LCD_WriteData('0' + numLives);
			LCD_AppendString(17, "Score: ");
			for (i = 24; i < 26; i++) {
				LCD_Cursor(i);
				LCD_WriteData('0' + score[j]);
				j++;
			}
			LCD_Cursor(26);
			LCD_WriteData('0');*/
			ReadGreen();
			ReadBlue();
			for (i = 0; i < 8; i++) {
				transmit_3_data(green[i], LED[i], blue[i]);
			}
			break;
	}
	return state;
};


/*------------------------- TASK 6: LCD DISPLAY -------------------------*/

int main(void)
{
    DDRA = 0xF0; PORTA = 0x0F;	// A0-3 are joystick input, A4-7 are shift reg output
    DDRB = 0xFF; PORTB = 0x00;	// B0-1 are control lines for LCD display
	DDRC = 0xF0; PORTC = 0x0F;	// Keypad input
	DDRD = 0xFF; PORTD = 0X00;	// Data lines for LCD display

    const unsigned short numTasks = 4;
    const unsigned short GCDPeriod = 25;
    task tasks[numTasks];
    unsigned char i = 0;

	/*// Test Task: LED Matrix Blue
	tasks[i].state = 0;
	tasks[i].period = 50;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &SM1_Tick;
	i++;
	// Test Task: LED Matrix Green
	tasks[i].state = 0;
	tasks[i].period = 75;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &SM2_Tick;*/
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
	tasks[i].period = 25;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &Level_Tick;
	i++;

	/*// Task 4: Ball
	tasks[i].state = 0;
	tasks[i].period = 50;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &Ball_Tick;
	i++;*/

	// Task 5: LEDDisplay
	tasks[i].state = 0;
	tasks[i].period = 25;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &LEDDisplay_Tick;
	i++;

	/*// Task 6: LCDDisplay
	tasks[i].state = 0;
	tasks[i].period = 100;
	tasks[i].elapsedTime = 0;
	tasks[i].TickFct = &Display_Tick;*/

    TimerSet(GCDPeriod);
    TimerOn();
	LCD_init();

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

