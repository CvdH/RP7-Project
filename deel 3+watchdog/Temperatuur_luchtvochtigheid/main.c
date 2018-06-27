/*
 * Temperatuur_luchtvochtigheid.cpp
 *
 * Created: 6/26/2018 12:52:06 PM
 * Author : Tulp
 */

 //#ifndef F_CPU
 #define F_CPU 16000000UL
 //#endif
 

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h" 
#include "freertos/queue.h"
#include "freertos/semphr.h" 
#include "freertos/serial.h"

//Start sonar stuff
#define BAUD 9600
#define MYUBRR (((F_CPU / (16UL * BAUD))) - 1)
#define TRIGGER PB0

#define RECEIVED_TRUE 1
#define RECEIVED_FALSE 0

#define INSTR_PER_US 16												// instructions per microsecond (depends on MCU clock, 16MHz current)
#define INSTR_PER_MS 16000											// instructions per millisecond (depends on MCU clock, 16MHz current)
#define MAX_RESP_TIME_MS 200										// timeout - max time to wait for low voltage drop
#define DELAY_BETWEEN_TESTS_MS 50									// echo cancelling time between sampling

void INT1_init( void );
void pulse( void );
void timer3_init( void );

char ontvang;														// Global variable to store received data
int state = RECEIVED_FALSE;
char out[256];

volatile uint8_t running = 0;										// State to see if the pulse has been send.
volatile uint8_t up = 0;
volatile uint32_t timerCounter = 0;
volatile unsigned long result = 0;
//end sonar stuff

//start servo stuff
//end servo stuff

//start main stuff
QueueHandle_t sonarAfstand;
QueueHandle_t servoHoek;
void initQ();
void readQ();
void writeQ();
void sonarTaak();
void servoTaak();
void temperatuurTaak();
void watchdogTaak();
void UART_Init();
void UART_Transmit(char data );
void UART_Transmit_String(char *stringPtr);
void UART_Transmit_Integer(uint32_t number);
void turnServo(uint8_t degrees);
void initServo();

void verzenden(uint8_t ad,uint8_t b);
void ontvangen(uint8_t ad,uint8_t b[],uint8_t max);
void init_master();

SemaphoreHandle_t sem;

int afstand;
int hoek;
int temperatuur;
int humidity;

int watchdogSonar, watchdogServo, watchdogTemp;

//end main stuff

//trigger = digital pin 53
//echo = digital pin 52
//servo = digital pin 12


int main() 
{
	sem = xSemaphoreCreateBinary();
	xSemaphoreGive(sem);
	DDRD|= 0x03;
	DDRB |= (1 << TRIGGER);											// Trigger pin
	UART_Init();
	INT1_init();
	timer3_init();
	initServo();
	sei();
	initQ();
	init_master();
	watchdogSonar=0;
	watchdogServo=0;
	watchdogTemp=0;
	wdt_enable(WDTO_4S);
	// Replace with your application code
	UART_Transmit_String("setup done\n\r");
	xTaskCreate(sonarTaak,"Sonar Sensor",256,NULL,3,NULL);			//lees sonar sensor uit en schrijf afstand naar sonar queue
	xTaskCreate(servoTaak,"Servo Motor",256,NULL,3,NULL);			//code van Joris & Benjamin
	xTaskCreate(temperatuurTaak,"temperatuur Sensor",256,NULL,3,NULL);
	xTaskCreate(watchdogTaak,"watchdog reset",256,NULL,4,NULL);

	vTaskStartScheduler();
}


void temperatuurTaak(){

	uint8_t data[2];
	uint32_t waarde =0;

	while(1){
		verzenden(0x40, 0xE3);
		ontvangen(0x40, data, 2);
		
		waarde = ((uint16_t)data[0]<<8) | data[1];
		
		temperatuur=((175.72*waarde)/65536.0) -46.85;

		verzenden(0x40, 0xE5);
		ontvangen(0x40, data, 2);
		
		waarde = ((uint16_t)data[0]<<8) | data[1];
		
		humidity=((125*waarde)/65536.0) -6;

		watchdogTemp = 1;
	}
}

void sonarTaak(){
	//UART_Transmit_String("taak uitgevoerd");
	//wdt_enable(WDTO_2S);											// enable watchdog timer at 2 seconds
	while(1)
	{
		if(running == 0)
		{
			//xSemaphoreTake(sem, portMAX_DELAY);
			_delay_ms(DELAY_BETWEEN_TESTS_MS);
			pulse();
			afstand = result;
			//xQueueSend(sonarAfstand, (void*) &result,0);
			//UART_Transmit_Integer(afstand);UART_Transmit_String("\n\r");
			//wdt_reset();
			//	xSemaphoreGive(sem);
		}

		watchdogSonar = 1;
	}
}


void servoTaak(){
	while(1){
		for(int i=0;i<9;i++){
			//xSemaphoreTake(sem, portMAX_DELAY);
			hoek = i*(250/9);
			turnServo(hoek);
			_delay_ms(250);
			UART_Transmit_String("hoek: ");
			UART_Transmit_Integer(hoek);
			UART_Transmit_String(" afstand: ");
			UART_Transmit_Integer(afstand);
			UART_Transmit_String(" temperatuur: ");
			UART_Transmit_Integer(temperatuur);
			UART_Transmit_String(" humidity: ");
			UART_Transmit_Integer(humidity);
			UART_Transmit_String("\n\r");
			//xSemaphoreGive(sem);
			//_delay_ms(5000);
			watchdogServo = 1;
		}
	}
}


void watchdogTaak(){
	while(1){
		_delay_ms(1);
		if(watchdogSonar&&watchdogServo&&watchdogTemp){
			UART_Transmit_String("WATCHDOG RESET\n\r");
			wdt_reset();
			watchdogSonar = 0;
			watchdogServo = 0;
			watchdogTemp = 0;
		}
	}
}

void wait(unsigned int a)
{
	while(a--)
		_delay_ms(50);
}

//main functions
void initQ(){
	sonarAfstand = xQueueCreate(10,sizeof(int));
	if(sonarAfstand==0) sonarAfstand = xQueueCreate(10,sizeof(int));

	servoHoek = xQueueCreate(10,sizeof(int));
	if(servoHoek==0) servoHoek = xQueueCreate(10,sizeof(int));
}

void writeQ( int data)
{
	xQueueSend(servoHoek, (void*) &hoek, 0);
}

void readQ()
{
	xQueueReceive(sonarAfstand, &afstand, 0);
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

//xSemaphoreTake(sem, portMAX_DELAY);
	while(*string)
	UART_Transmit(*string++);
	
//	xSemaphoreGive(sem);
}

void UART_Transmit_Integer(uint32_t number)
{
//	xSemaphoreTake(sem, portMAX_DELAY);
	char buffer[17];
	itoa(number, &buffer[0], 10);
	UART_Transmit_String(&buffer[0]);
//	xSemaphoreGive(sem);
}

ISR(USART0_RX_vect)
{
	ontvang = UDR0;
	UART_Transmit(ontvang);
	state = RECEIVED_TRUE;
}

//Sonar functions

void INT1_init()
{
	PCICR |= (1<<PCIE0);
	PCMSK0 |= (1<<PCINT1);
}
ISR(PCINT0_vect)
{
	if(running)														// check if the pulse has been send.
	{
		if(up == 0)													// voltage rise
		{
			up = 1;
			timerCounter = 0;
			TCNT3 = 0;
		}
		else														// faling edge
		{
			up = 0;
			result = ((timerCounter * 65535 + TCNT3) / 58) / 2;
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

void timer3_init()
{
	TCCR3B |= (0 << CS30) | (1 << CS31) | (0 << CS32);				// prescaler 1024 so we get 15625Hz clock ticks. or 4.19 s per tick.
	TCNT3 = 0;														// init counter
	TIMSK3 |= (1 << TOIE3);											// enable overflow interrupt
}

ISR(TIMER3_OVF_vect)
{
	if(up)
	{
		timerCounter++;
		uint32_t ticks = timerCounter * 65535 + TCNT3;
		uint32_t maxTicks = (uint32_t)MAX_RESP_TIME_MS * INSTR_PER_MS;
		if(ticks > maxTicks)
		{
			up = 0;
			running = 0;
			result = -1;
		}
	}
}

void turnServo(uint8_t degrees)
{
	OCR1B = 20000 - (degrees * (1300 / 180) + 800);
}

void initServo()
{
	DDRB |= (1 << PB6);
	TCCR1A = (1 << WGM11)| (1 << COM1B0) | (1 << COM1B1);
	TCCR1B = (1 << WGM13) | (1 << CS11);
	ICR1 = 20000;
	TCNT1 = 0;
	turnServo(0);
	TIMSK1 |= (1 << 1);
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

ISR(TIMER1_COMPA_vect)
{
	PORTB ^= (1 << PB6);
}

 /*
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
    // Replace with your application code 
    while (1) 
    {
	    uint8_t data[2];
	    uint32_t waarde =0;
	
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
// tempratuur in C = ((175.72*Temp_code)/65536) -46.85;\

*/