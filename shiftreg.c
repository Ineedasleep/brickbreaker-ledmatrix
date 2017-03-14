/* Implementation file for Shift Register Logic
 *
 * Ports currently being used:
 * SER: A4
 * SRCLR: A7
 * SRCLK: A6
 * RCLK: A5
 *
 */

 // transmits to 3 daisy-chained shift registers connected to A4-7
 void transmit_3_data(unsigned char data1, unsigned char data2, unsigned char data3) {
	 /*---------------- Serial read ----------------*/
	 int i;
	 PORTA |= 0x80;
	 // First char
	 for (i = 7; i >= 0; i--) {
		 // Sets SRCLR(A7) to 1 allowing data to be set
		 // Also clears SRCLK(A6) in preparation of sending data
		 PORTA &= 0x8F;
		 // set SER(A4) = next bit of data to be sent
		 PORTA |= (((data1>>i) & 0x01) << 4);
		 // set SRCLK(A6) = 1. Rising edge shifts next bit of data into the shift register
		 PORTA |= 0x50;
	 }
	 // Second char
	 for (i = 7; i >= 0; i--) {
		 // Sets SRCLR(A7) to 1 allowing data to be set
		 // Also clears SRCLK(A6) in preparation of sending data
		 PORTA &= 0x8F;
		 // set SER(A4) = next bit of data to be sent
		 PORTA |= (((data2>>i) & 0x01) << 4);
		 // set SRCLK(A6) = 1. Rising edge shifts next bit of data into the shift register
		 PORTA |= 0x50;
	 }
	 // Third char
	 for (i = 7; i >= 0; i--) {
		 // Sets SRCLR(A7) to 1 allowing data to be set
		 // Also clears SRCLK(A6) in preparation of sending data
		 PORTA &= 0x8F;
		 // set SER(A4) = next bit of data to be sent
		 PORTA |= (((data3>>i) & 0x01) << 4);
		 // set SRCLK(A6) = 1. Rising edge shifts next bit of data into the shift register
		 PORTA |= 0x50;
	 }

	 /*---------------- Parallel Load ----------------*/
	 // set RCLK(A5) = 1. Rising edge copies data from "Shift" register to "Storage" register
	 PORTA |= 0x20;
	 // clears all lines in preparation of a new transmission
	 PORTA = 0x00;
 }