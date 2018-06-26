/*
 * Temperatuur_luchtvochtigheid.cpp
 *
 * Created: 6/26/2018 12:52:06 PM
 * Author : Tulp
 */

#define F_CPU 16000000UL

#include <avr/io.h>
#include <stdlib.h>

#define BAUD 9600
#define MYUBRR (((F_CPU / (16UL * BAUD))) - 1)

void UART_Init( unsigned int ubrr );
void UART_Transmit( unsigned char data );
void UART_Transmit_String(const char *stringPtr);
void UART_Transmit_Integer(int16_t number, uint8_t base);

void verzenden(uint8_t ad,uint8_t b);
void ontvangen(uint8_t ad,uint8_t b[],uint8_t max);
void init_master();

int main(void)
{
	PORTD = 0x03;
	UART_Init(MYUBRR);
	init_master();
    /* Replace with your application code */
	uint8_t data[16];
    while (1) 
    {
		verzenden(0x40, 0xE5);
		ontvangen(0x40, data, 2);
		//UART_Transmit_Integer(data[0],10);
    }
}

void init_master() {
	TWSR = 0;
	// Set bit rate
	TWBR = ( ( F_CPU / 100000 ) - 16) / 2;
	TWCR = (1<<TWEN);
}

void ontvangen(uint8_t ad,uint8_t b[],uint8_t max) {
	uint8_t op[15];
	
	UART_Transmit('a');

	TWCR |= (1<<TWSTA);
	while(!(TWCR & (1<<TWINT)));
	op[0] = TWSR;

	
	UART_Transmit('b');

	TWDR=(ad<<1)+1;
	TWCR=(1<<TWINT)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));

	op[1] = TWSR;
	b[0]=TWDR;
	
	uint8_t tel=0;
	do{
		
		//UART_Transmit('c');
		if(tel == max-1){
			TWCR=(1<<TWINT)|(1<<TWEN);
			//UART_Transmit('c1');
		}else{
			TWCR=(1<<TWINT)|(1<<TWEN)|(1<<TWEA);
			//UART_Transmit('c2');
		}
		while(!(TWCR & (1<<TWINT))); //blijft hier vastzitten
		//UART_Transmit('c3');
		op[tel] = TWSR;
		b[tel]=TWDR;
	}while(op[tel++] == 0x50);

	
	//UART_Transmit('d');

	TWCR=(1<<TWINT)|(1<<TWSTO)|(1<<TWEN);

	//   for(uint8_t i=0;i<tel;++i) {
	//	 writeString("\n\r");writeInteger(op[i],16);
	//	 writeString(" data ");writeInteger(b[i],10);
	//   }
}

void verzenden(uint8_t ad,uint8_t b) {
	//  uint8_t op[5];

	TWCR |= (1<<TWSTA);
	while(!(TWCR & (1<<TWINT)));
	//   op[0] = TWSR;
	TWDR=(ad<<1);
	TWCR=(1<<TWINT)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
	//    op[1] = TWSR;

	TWDR=b;
	TWCR=(1<<TWINT)|(1<<TWEN);
	while(!(TWCR & (1<<TWINT)));
	//  op[2] = TWSR;

	TWCR=(1<<TWINT)|(1<<TWSTO)|(1<<TWEN);
	//	while(!(TWCR & (1<<TWINT)));
	//  for(uint8_t i=0;i<3;++i) {
	// writeString("\n\r");writeInteger(op[0],16);
	// writeString(" ");writeInteger(op[1],16);
	// writeString(" ");writeInteger(op[2],16);
}


void UART_Init( unsigned int ubrr)
{
	UBRR0H = (ubrr>>8);												// Set baud rate
	UBRR0L = ubrr;

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


void UART_Transmit_Integer(int16_t number, uint8_t base)
{
	char buffer[17];
	itoa(number, &buffer[0], base);
	UART_Transmit_String(&buffer[0]);
}

// humidity in % = ((125*RH_CODE)/65536) -6;
// tempratuur in C = ((175.72*Temp_code)/65536) -46.85;