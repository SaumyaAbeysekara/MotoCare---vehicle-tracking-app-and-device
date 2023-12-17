/*
 * LCD_20x4_H_file.h
 * http://www.electronicwings.com
 *
 */

#ifndef LCD_20x4_H_H_					/* Define library H file if not defined */
#define LCD_20x4_H_H_

#define F_CPU 8000000UL					/* Define CPU Frequency e.g. here its 8MHz */
#include <avr/io.h>						/* Include AVR std. library file */
#include <util/delay.h>					/* Include Delay header file */

#define LCD_Data_Dir DDRB				/* Define LCD data port direction */
#define LCD_Command_Dir DDRC			/* Define LCD command port direction register */
#define LCD_Data_Port PORTB				/* Define LCD data port */
#define LCD_Command_Port PORTC			/* Define LCD data port */
#define EN PC2							/* Define Enable signal pin */
#define RW PC1							/* Define Read/Write signal pin */
#define RS PC0							/* Define Register Select (data reg./command reg.) signal pin */

void LCD_Command (char);				/* LCD command write function */
void LCD_Char (char);					/* LCD data write function */
void LCD_Init (void);					/* LCD Initialize function */
void LCD_String (char*);				/* Send string to LCD function */
void LCD_String_xy (char,char,char*);	/* Send row, position and string to LCD function */
void LCD_Clear (void);					/* LCD clear function */

void LCD_Command(char cmd) {
	LCD_Data_Port = cmd;
	LCD_Command_Port &= ~((1 << RS) | (1 << RW));
	LCD_Command_Port |= (1 << EN);
	_delay_us(1);
	LCD_Command_Port &= ~(1 << EN);
	_delay_ms(3);
}

void LCD_Char(char char_data) {
	LCD_Data_Port = char_data;
	LCD_Command_Port &= ~(1 << RW);
	LCD_Command_Port |= (1 << EN) | (1 << RS);
	_delay_us(1);
	LCD_Command_Port &= ~(1 << EN);
	_delay_ms(1);
}

void LCD_Init(void) {
	LCD_Command_Dir |= (1 << RS) | (1 << RW) | (1 << EN);
	LCD_Data_Dir = 0xFF;

	_delay_ms(20);
	LCD_Command(0x38);
	LCD_Command(0x0C);
	LCD_Command(0x06);
	LCD_Command(0x01);
	LCD_Command(0x80);
}

void LCD_String(char *str) {
	int i;
	for (i = 0; str[i] != 0; i++) {
		LCD_Char(str[i]);
	}
}

void LCD_String_xy(char row, char pos, char *str) {
	if (row == 1)
	LCD_Command((pos & 0x0F) | 0x80);
	else if (row == 2)
	LCD_Command((pos & 0x0F) | 0xC0);
	else if (row == 3)
	LCD_Command((pos & 0x0F) | 0x94);
	else if (row == 4)
	LCD_Command((pos & 0x0F) | 0xD4);
	LCD_String(str);
}

void LCD_Clear(void) {
	LCD_Command(0x01);
	LCD_Command(0x80);
}


#endif									/* LCD_20x4_H_FILE_H_ */