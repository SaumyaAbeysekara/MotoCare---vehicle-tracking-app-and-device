/*
 * USART_Interrupt.h
 *
 * Created: 05/09/2016 4:57:37 PM
 *  Author: amrut
 */ 


#ifndef USART_INTERRUPT_H_
#define USART_INTERRUPT_H_
#define F_CPU 8000000UL						/* Define CPU clock Frequency e.g. here its 8MHz */
#include <avr/io.h>							/* Include AVR std. library file */
#include <avr/interrupt.h>
#include "USART_Interrupt.h"

#define BAUD_PRESCALE (((F_CPU / (BAUDRATE * 16UL))) - 1)	/* Define prescale value */

void USART_Init(unsigned long);				/* USART initialize function */
char USART_RxChar(void);						/* Data receiving function */
void USART_TxChar(char);					/* Data transmitting function */
void USART_SendString(char *str);




void UART_Init(unsigned long BAUDRATE) {
	UCSRB |= (1 << RXEN) | (1 << TXEN) | (1 << RXCIE);
	UCSRC |= (1 << URSEL) | (1 << UCSZ0) | (1 << UCSZ1);
	UBRRL = BAUD_PRESCALE;
	UBRRH = (BAUD_PRESCALE >> 8);
}

char UART_RxChar(void) {
	while (!(UCSRA & (1 << RXC)));
	return (UDR);
}

void UART_TxChar(char data) {
	UDR = data;
	while (!(UCSRA & (1 << UDRE)));
}

void USART_SendString(char *str)					/* Send string of USART data function */
{
	int i=0;
	while (str[i]!=0)
	{
		UART_TxChar(str[i]);						/* Send each char of string till the NULL */
		i++;
	}
}



#endif /* USART_INTERRUPT_H_ */
