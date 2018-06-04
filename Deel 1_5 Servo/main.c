

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
#define TRIGGER PB0 // = digital 53
#define INSTR_PER_US 16												// instructions per microsecond (depends on MCU clock, 16MHz current)
#define INSTR_PER_MS 16000											// instructions per millisecond (depends on MCU clock, 16MHz current)
#define MAX_RESP_TIME_MS 200										// timeout - max time to wait for low voltage drop
#define DELAY_BETWEEN_TESTS_MS 50

/*#define TRIGGER PB0
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
#define PIN_GELUID PINA*/

//SERVO OP PB6 = DIGITAL 12
//SONAR TRIGGER OP PB0 = DIGITAL 53
//SONAR ECHO OP PB1 = DIGITAL 52

void UART_Init( unsigned int ubrr );
void UART_Transmit( unsigned char data );
void UART_Transmit_String(const char *stringPtr);

//SERVO
void turnServo(int degrees);
void initServo();
void addToBuffer(char x);

//ULTRASOON
void INT1_init( void );
void pulse( void );
void timer1_init( void );

char ontvang;														// Global variable to store received data
//int state = RECEIVED_FALSE;
char out[256];

volatile uint8_t running = 0;										// State to see if the pulse has been sent.
volatile uint8_t up = 0;
volatile uint32_t timerCounter = 0;
volatile unsigned long result = 0;

char buffer[3];
int ind;

//using namespace std;

int main()
{
	DDRB = (1 << TRIGGER);
	DDRB |= (1 << PB7);
	UART_Init(MYUBRR);
	initServo();
	INT1_init();
	//wdt_enable(WDTO_2S);
	timer1_init();
	sei();
	UART_Transmit_String("Setup done");
	while(1)
	{
		if(running == 0)
		{
			_delay_ms(DELAY_BETWEEN_TESTS_MS);
			pulse();
			sprintf(out, "Afstand = %dCM", result);
			UART_Transmit_String(out);
			if(result > 250) result = 250;
			turnServo(result);
		}
	}
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
		ind=0;
	}else{
		buffer[ind++]=x;
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

 void INT1_init()
 {
	 EICRA |= (1 << ISC10) | (0 << ISC11);							// set rising or falling edge on INT1
	 EIMSK |= (1 << INT1);											// Enable INT1
 }

 ISR(INT1_vect)
 {
	PORTB |= (1 << PB7);
	 if(running)														// check if the pulse has been sent
	 {
		 if(up == 0)													// voltage rise
		 {
			 up = 1;
			 timerCounter = 0;
			 TCNT1 = 0;
		 }
		 else														// faling edge
		 {
			 wdt_reset();
			 up = 0;
			 result = ((timerCounter * 65535 + TCNT1) / 58) / 2;
			 running = 0;
		 }
	 }
 }

void pulse()
{
	PORTB &= ~(1 << TRIGGER);
	_delay_us(1);


	PORTB |= (1 << TRIGGER);										// zet trigger op 1
	running = 1;
	_delay_us(10);													// wacht 10 microseconden
	PORTB &= ~(1 << TRIGGER);										// zet trigger op 0
}

void timer1_init()
{
	TCCR1B |= (0 << CS10) | (1 << CS11) | (0 << CS12);				// prescaler 0
	TCNT1 = 0;														// init counter
	TIMSK1 |= (1 << TOIE1);											// enable overflow interrupt
}

ISR(TIMER1_OVF_vect)
{
	if(up)
	{
		timerCounter++;
		uint32_t ticks = timerCounter * 65535 + TCNT1;
		uint32_t maxTicks = (uint32_t)MAX_RESP_TIME_MS * INSTR_PER_MS;
		if(ticks > maxTicks)
		{
			up = 0;
			running = 0;
			result = -1;
		}
		
	}
}
