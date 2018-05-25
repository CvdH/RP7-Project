/*
 * sonarsensort.cpp
 *
 * Created: 5/25/2018 11:36:00 AM
 * Author : Tulp
 */ 

 #ifndef F_CPU
 #define F_CPU 16000000UL
 #endif

 #include <avr/io.h>
 #include <util/delay.h>
 #include <avr/interrupt.h>


 #define BAUD 9600
 #define MYUBRR (((F_CPU / (16UL * BAUD))) - 1)

 #define RECEIVED_TRUE 1
 #define RECEIVED_FALSE 0
 #define trigger PINB0
 #define echo PINB2

 void wait(unsigned int);
 void UART_Init();
 unsigned char UART_Receive( void );
 void UART_Transmit( unsigned char data );
 void triggerPin();
 void echoPin();

 char ontvang; // global variable to store received data
 int state = RECEIVED_FALSE;


int main(void)
{

	DDRB |= 1<<trigger;
	DDRB &= ~(1<<echo);

	UART_Init();

    UART_Transmit('b');


    /* Replace with your application code */
    while (1) 
    {
	    triggerPin();
		echoPin();
    }
}


void triggerPin(){
	PORTB |= 1<<trigger;
	_delay_ms(10);
	PORTB &= ~1<<trigger;
	UART_Transmit('s');
}

void echoPin(){
	while(!(PINB & 1<<echo));
	UART_Transmit('r');
}


void UART_Init()
{
	UBRR0H = (MYUBRR>>8);												// Set baud rate
	UBRR0L = MYUBRR;

	UCSR0B = (1<<RXEN0) | (1<<TXEN0) | (1<<RXCIE0);					// Enable receiver and transmitter and enable RX interrupt
	UCSR0C = (3<<UCSZ00);											// Set frame format: 8data, 1 stop bit
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

unsigned char UART_Receive( void )
{
	while ( !(UCSR0A & (1<<RXC0)) )	{};								// Wait for data to be received
	return UDR0;													// Get and return received data from buffer
}
