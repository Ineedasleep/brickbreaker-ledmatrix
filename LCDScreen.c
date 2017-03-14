/* Implementation file for LCDScreen Logic
 *
 *
 *
 *
 */

#include "LCD.c"

void LCD_AppendString( unsigned char column, const unsigned char* string) {
	unsigned char c = column;
	while(*string) {
		LCD_Cursor(c++);
		LCD_WriteData(*string++);
	}
}