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

void UART_Init();
void UART_Transmit(char data );
void UART_Transmit_String(char *stringPtr);
void UART_Transmit_Integer(uint32_t number);

void verzenden(uint8_t ad,uint8_t b);
void ontvangen(uint8_t ad,uint8_t b[],uint8_t max);
void init_master();

int main(void)
{
	PORTD = 0x03;
	UART_Init();
	init_master();
    /* Replace with your application code */
	uint8_t data[2];
	uint32_t waarde =0;
    while (1) 
    {
	
		verzenden(0x40, 0xE3);
		ontvangen(0x40, data, 2);
	
		waarde = ((uint16_t)data[0]<<8) | data[1];
	
		UART_Transmit_String("temperatuur: ");
		UART_Transmit_Integer(((175.72*waarde)/65536.0) -46.85);

		verzenden(0x40, 0xE5);
		ontvangen(0x40, data, 2);
		
		waarde = ((uint16_t)data[0]<<8) | data[1];
		
		UART_Transmit_String(" humidity: ");
		UART_Transmit_Integer(((125*waarde)/65536.0) -6);
		UART_Transmit_String("\n\r");
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
	
//	UART_Transmit('a');

	TWCR |= (1<<TWSTA);
	while(!(TWCR & (1<<TWINT)));
	op[0] = TWSR;

	
	//UART_Transmit('b');

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

void UART_Init() {

	UBRR0H = MYUBRR >> 8;
	UBRR0L = (uint8_t) MYUBRR;
	UCSR0A = 0x00;
	UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
	UCSR0B = (1 << TXEN0) | (1 << RXEN0);
}

void UART_Transmit(char ch)
{
	while (!(UCSR0A & (1<<UDRE0)));
	UDR0 = (uint8_t)ch;
}

void UART_Transmit_String(char *string)
{
	while(*string)
	UART_Transmit(*string++);
}

void UART_Transmit_Integer(uint32_t number)
{
	char buffer[17];
	itoa(number, &buffer[0], 10);
	UART_Transmit_String(&buffer[0]);
}



// humidity in % = ((125*RH_CODE)/65536) -6;
// tempratuur in C = ((175.72*Temp_code)/65536) -46.85;