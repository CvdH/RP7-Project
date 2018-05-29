

#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>


//Start sonar stuff
#define BAUD 9600
#define MYUBRR (((F_CPU / (16UL * BAUD))) - 1)
#define TRIGGER PB0
#define GELUID PB1

#define RECEIVED_TRUE 1
#define RECEIVED_FALSE 0

#define INSTR_PER_US 16												// instructions per microsecond (depends on MCU clock, 16MHz current)
#define INSTR_PER_MS 16000											// instructions per millisecond (depends on MCU clock, 16MHz current)
#define MAX_RESP_TIME_MS 200										// timeout - max time to wait for low voltage drop
#define DELAY_BETWEEN_TESTS_MS 50									// echo cancelling time between sampling

#define L_PLUS PC1
#define L_MIN PD7
#define L_EN PL5

#define R_PLUS PG1
#define R_MIN PL7
#define R_EN PL3
#define SERVO_PIN PL1

#define L_GELUID PA3
#define R_GELUID PA1
#define PIN_GELUID PINA


void UART_Init( unsigned int ubrr );
void UART_Transmit( unsigned char data );
void UART_Transmit_String(const char *stringPtr);
void turnServo(int degrees);
void initServo();
void addToBuffer(char x);

char buffer[3];
int index;

using namespace std;

int main()
{
	UART_Init(MYUBRR);
	initServo();
	sei();
	UART_Transmit_String("Setup done");
	while(1);
}

void turnServo(int degrees){
	OCR1B = 20000 - (degrees * (1300 / 180) + 800);
}

void addToBuffer(char x){
	if(x=='\r'){
		UART_Transmit('\n');
		int temp;
		temp = atoi (buffer);
		turnServo(temp);
		buffer[0] = '\0';
		buffer[1] = '\0';
		buffer[2] = '\0';
		index=0;
	}else{
		buffer[index++]=x;
	}
	UART_Transmit(x);
}


void UART_Transmit( unsigned char data )
{
	while ( !( UCSR0A & (1<<UDRE0)) ) {};							// Wait for empty transmit buffer
	UDR0 = data;													// Put data into buffer, sends the data
}

void UART_Transmit_String(const char *stringPtr)
{
	while(*stringPtr != 0x00)
	{
		UART_Transmit(*stringPtr);
		stringPtr++;
	}
	
	UART_Transmit('\n');
	UART_Transmit('\r');
}

void UART_Init( unsigned int ubrr)
{
	UBRR0H = (ubrr>>8);												// Set baud rate
	UBRR0L = ubrr;

	UCSR0B = (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0);					// Enable receiver and transmitter and enable RX interrupt
	UCSR0C = (3<<UCSZ00);											// Set frame format: 8data, 1 stop bit
}

ISR(USART0_RX_vect)
{
	addToBuffer(UDR0);
	//UART_Transmit_String("interupt    ");
}


void initServo()
{
	DDRB |= (1 << PB6);
	TCCR1A = (1 << WGM11) | (1 << COM1B0) | (1 << COM1B1);
	TCCR1B = (1 << WGM13) | (1 << CS11);
	ICR1 = 20000;
	TCNT1 = 0;
	turnServo(0);
}
