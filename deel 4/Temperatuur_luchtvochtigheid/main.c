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
#include <stdbool.h>

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

#define L_PLUS PC1
#define L_MIN PD7
#define L_EN PL5

#define R_PLUS PG1
#define R_MIN PL7
#define R_EN PL3


//pwm pinnen = digital pin 46,44,42,40,38,36 = PL3, PL5, PL7, PG1, PD7, PC1

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
void sonarTaak();
void servoTaak();
void temperatuurTaak();
void gyroTaak();
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

void motorTaak();
void motorTaak2();
void initMotor();
void setSpeed(uint8_t speed);
void motorVooruit();
void motorAchteruit();
void motorRechts();
void motorLinks();
void motorEnable();
void motorStop();
QueueHandle_t motorCommand;
char send[20];

SemaphoreHandle_t sem;

int afstand;
int hoek;
int temperatuur;
int humidity;
int accelX;
int accelY;
int accelZ;
int gyroX;
int gyroY;
int gyroZ;
int temp;

const float G = 9.807f;
const float _d2r = 3.14159265359f/180.0f;
float _accelScale;
float _gyroScale;

int watchdogSonar, watchdogServo, watchdogTemp, watchdogGyro, watchdogMotor;

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
	initMotor();
	motorCommand = xQueueCreate(10, sizeof(uint8_t));
	UART_Init();
	INT1_init();
	timer3_init();
	initServo();
	sei();
	//initQ();
	init_master();
	watchdogSonar=0;
	watchdogServo=0;
	watchdogTemp=0;
	watchdogMotor=0;
	wdt_enable(WDTO_4S);
	_accelScale = G * 16.0f/32767.5f;
	_gyroScale = 2000.0f/32767.5f * _d2r;

	// Replace with your application code
//	UART_Transmit('a');
	UART_Transmit_String("\n\rsetup done\n\r");
	xTaskCreate(sonarTaak,"Sonar Sensor",256,NULL,3,NULL);			//lees sonar sensor uit en schrijf afstand naar sonar queue
	xTaskCreate(servoTaak,"Servo Motor",256,NULL,3,NULL);			//code van Joris & Benjamin
	//xTaskCreate(temperatuurTaak,"temperatuur Sensor",256,NULL,3,NULL);
	xTaskCreate(gyroTaak,"Gyroscoop Sensor",256,NULL,3,NULL);
	xTaskCreate(motorTaak,"Motor Taak met input",256,NULL,3,NULL);
	//xTaskCreate(motorTaak2,"Motor Taak zonder input",256,NULL,3,NULL);
	xTaskCreate(watchdogTaak,"watchdog reset",256,NULL,4,NULL);

	vTaskStartScheduler();
}

void motorTaak2(){
	setSpeed(20);
	const TickType_t xDelay = 2000 / portTICK_PERIOD_MS;
	while(1){
		setSpeed(20);
		motorVooruit();	
		vTaskDelay( xDelay );
		motorAchteruit();
		vTaskDelay( xDelay );
		setSpeed(100);
		motorLinks();
		vTaskDelay(xDelay);
		motorRechts();
		vTaskDelay(xDelay);
		watchdogMotor = 1;
	}
}

void motorTaak(){
	uint8_t temp;
	int16_t targetSpeed = 50;
	int16_t currentSpeed = 20;
	bool riding = false;
	bool turning = false;
	setSpeed(currentSpeed);

	while (1){
		/*if (xQueueReceive(motorCommand, &temp, 0)){
			UART_Transmit(temp);*/
			switch(ontvang){
				case 'w':
					//currentSpeed = 0;
					setSpeed(currentSpeed);
					riding = true;
					turning = false;
					motorStop();
					motorVooruit();
					break;
				case 'a':
					riding = true;
					turning = true;
					//if(currentSpeed <= 50) setSpeed(50);
					motorStop();
					motorLinks();
					break;
				case 's':
					//currentSpeed = 0;
					setSpeed(currentSpeed);
					riding = true;
					turning = false;
					motorStop();
					motorAchteruit();
					break;
				case 'd':
					riding = true;
					turning = true;
					if(currentSpeed <= 50) setSpeed(50);
					motorStop();
					motorRechts();
					break;
				case 'x':
					riding = false;
					turning = false;
					motorStop();
					break;
				/*case 'A':
					motorLinks();
					vTaskDelay(100);
					motorStop();
					break;
				case 'D':
					motorRechts();
					vTaskDelay(100);
					motorStop();
					break;*/
				case '+':
					targetSpeed += 10;
					if (targetSpeed > 100) targetSpeed = 100;
					break;
				case '-':
					targetSpeed -= 10;
					if (targetSpeed < 0) targetSpeed = 0;
					break;
				default:
					//nothing
					break;	
				
			}
			ontvang = '0';

		if (riding){
			if (turning){
				setSpeed(50);
			}
			else if (currentSpeed < targetSpeed){
				currentSpeed += 5;
				if (currentSpeed > 100) currentSpeed = 100;
				setSpeed(currentSpeed);
				vTaskDelay(25);
			}
			else if(currentSpeed > targetSpeed){
				currentSpeed -= 5;
				if (currentSpeed < 20) currentSpeed = 20;
				setSpeed(currentSpeed);
				vTaskDelay(25);
			}
			else{
				setSpeed(currentSpeed);
				vTaskDelay(25);
			}
		}
		watchdogMotor = 1;
	}
}

void gyroTaak(){

	uint8_t data[2];
	uint16_t waarde =0;

	while(1){
		// I2C adres van Gyro = 0x68 of 0c69
		uint8_t waardeH = 0;
		uint8_t waardeL = 0;

		//gyro X
		verzenden(0x68, 0x3B);
		ontvangen(0x68, data, 1);
		waardeH = data[0];
		verzenden(0x68, 0x3C);
		ontvangen(0x68, data, 1);
		waardeL = data[0];
		waarde = ((uint16_t)waardeH<<8) | waardeL;
		gyroX = waarde * _gyroScale - 69;

		//gyro Y
		verzenden(0x68, 0x3D);
		ontvangen(0x68, data, 1);
		waardeH = data[0];
		verzenden(0x68, 0x3E);
		ontvangen(0x68, data, 1);
		waardeL = data[0];
		waarde = ((uint16_t)waardeH<<8) | waardeL;
		gyroY = waarde * _gyroScale;

		// gyro Z
		verzenden(0x68, 0x3F);
		ontvangen(0x68, data, 1);
		waardeH = data[0];
		verzenden(0x68, 0x40);
		ontvangen(0x68, data, 1);
		waardeL = data[0];
		waarde = ((uint16_t)waardeH<<8) | waardeL;
		gyroZ = waarde * _gyroScale - 16;

		//accel X
		verzenden(0x68, 0x43);
		ontvangen(0x68, data, 1);
		waardeH = data[0];
		verzenden(0x68, 0x44);
		ontvangen(0x68, data, 1);
		waardeL = data[0];
		waarde = ((uint16_t)waardeH<<8) | waardeL;
		accelX = waarde * _accelScale;
		
		//accel Y
		verzenden(0x68, 0x45);
		ontvangen(0x68, data, 1);
		waardeH = data[0];
		verzenden(0x68, 0x46);
		ontvangen(0x68, data, 1);
		waardeL = data[0];
		waarde = ((uint16_t)waardeH<<8) | waardeL;
		accelY = waarde * _accelScale;

		//accel Z
		verzenden(0x68, 0x47);
		ontvangen(0x68, data, 1);
		waardeH = data[0];
		verzenden(0x68, 0x48);
		ontvangen(0x68, data, 1);
		waardeL = data[0];
		waarde = ((uint16_t)waardeH<<8) | waardeL;
		accelZ = waarde * _accelScale - 313;

		//temp
		/*verzenden(0x68, 0x41);
		ontvangen(0x68, data, 1);
		waardeH = data[0];
		verzenden(0x68, 0x42);
		ontvangen(0x68, data, 1);
		waardeL = data[0];
		waarde = ((uint16_t)waardeH<<8) | waardeL;
		temp = waarde;*/

		watchdogGyro = 1;
	}
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
	while(1){
		if(running == 0){
			_delay_ms(DELAY_BETWEEN_TESTS_MS);
			pulse();
			afstand = result;
		}
		watchdogSonar = 1;
	}
}


void servoTaak(){
	while(1){
		for(int i=0;i<8;i++){
			hoek = i*(250/9);
			turnServo(hoek);
			_delay_ms(250);
			UART_Transmit_String("\n\nHoek: ");
			UART_Transmit_Integer(hoek);
			UART_Transmit_String(" Afstand: ");
			UART_Transmit_Integer(afstand);

			UART_Transmit_String("\nGyro X: ");
			UART_Transmit_Integer(gyroX);
			UART_Transmit_String(" Y: ");
			UART_Transmit_Integer(gyroY);
			UART_Transmit_String(" Z: ");
			UART_Transmit_Integer(gyroZ);

			UART_Transmit_String("\nAccel X: ");
			UART_Transmit_Integer(accelX);
			UART_Transmit_String(" Y: ");
			UART_Transmit_Integer(accelY);
			UART_Transmit_String(" Z: ");
			UART_Transmit_Integer(accelZ);

			//UART_Transmit_String(" TEMP: ");
			//UART_Transmit_Integer(temp);

			/*UART_Transmit_String(" humidity: ");
			UART_Transmit_Integer(humidity);*/

			//UART_Transmit_String("\n\r");
			//_delay_ms(5000);
			watchdogServo = 1;
		}
	}
}


void watchdogTaak(){
	while(1){
		_delay_ms(1);
		if(watchdogSonar && watchdogServo && watchdogGyro&&watchdogMotor){
			//UART_Transmit_String("WATCHDOG RESET\n\r");
			wdt_reset();
			watchdogSonar = 0;
			watchdogServo = 0;
			watchdogGyro = 0;
			watchdogMotor = 0;
			//watchdogTemp = 0;
		}
	}
}

void wait(unsigned int a){
	while(a--)
		_delay_ms(50);
}

void UART_Init() {
	UBRR1H = MYUBRR >> 8;
	UBRR1L = (uint8_t) MYUBRR;
	UCSR1A = 0x00;
	UCSR1C = (1<<UCSZ11)|(1<<UCSZ10);
	//UCSR1B = (1 << TXEN1) | (1 << RXEN1);
	UCSR1B = (1 << RXEN1) | (1 << TXEN1) | 
		(1 << RXCIE1 ) | (1 << UDRIE1);
}

void UART_Transmit(char ch){
	while (!(UCSR1A & (1<<UDRE1)));
	UDR1 = (uint8_t)ch;
}

ISR(USART1_RX_vect){
	ontvang = UDR1;
	//UART_Transmit_String("Iets ontvangen!!!!!\n\r");
	//UART_Transmit(ontvang);
	//xQueueSend(motorCommand, (void*) &ontvang, 0);
	//state = RECEIVED_TRUE;
}

void UART_Transmit_String(char *string){
	while(*string)
	UART_Transmit(*string++);
}

void UART_Transmit_Integer(uint32_t number){
	char buffer[17];
	itoa(number, &buffer[0], 10);
	UART_Transmit_String(&buffer[0]);
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

//MOTOR STUFF
void initMotor()
{

//pwm pinnen = digital pin 46,44,42,40,38,36 = PL3, PL5, PL7, PG1, PD7, PC1

/*
#define L_PLUS PC1
#define L_MIN PD7
#define L_EN PL5 -> pwm

#define R_PLUS PG1
#define R_MIN PL7
#define R_EN PL3 -> pwm
*/
	DDRC |= (1 << L_PLUS);
	DDRD |= (1 << L_MIN);
	DDRL |= (1 << L_EN) | (1 << R_MIN) | (1 << R_EN);
	DDRG |= (1 << R_PLUS);

	TCCR5A = (1 << WGM51) | (1 << COM5A0) | (1 << COM5A1) |
	(1 << COM5C0) | (1 << COM5C1);
	TCCR5B = (1 << WGM53) | (1 << CS51);
	ICR5 = 20000;
	TCNT5 = 0;

	motorEnable();
}

void motorAchteruit()
{
	PORTL &= ~(1 << R_MIN);
	PORTD &= ~(1 << L_MIN);
	PORTG |= (1 << R_PLUS);
	PORTC |= (1 << L_PLUS);
	watchdogMotor = 1;
}

void motorVooruit()
{
	PORTG &= ~(1 << R_PLUS);
	PORTC &= ~(1 << L_PLUS);
	PORTL |= (1 << R_MIN);
	PORTD |= (1 << L_MIN);
	watchdogMotor = 1;
}

void motorRechts()
{
	PORTD &= ~(1 << L_MIN);
	PORTG &= ~(1 << R_PLUS);
	PORTL |= (1 << R_MIN);
	PORTC |= (1 << L_PLUS);
	watchdogMotor = 1;
}

void motorLinks()
{
	PORTC &= ~(1 << L_PLUS);
	PORTL &= ~(1 << R_MIN);
	PORTG |= (1 << R_PLUS);
	PORTD |= (1 << L_MIN);
	watchdogMotor = 1;
}

void motorEnable()
{
	motorStop();
	PORTL |= (1 << R_EN);
	PORTL |= (1 << L_EN);
}

void motorStop()
{
	PORTG &= ~(1 << R_PLUS);
	PORTL &= ~(1 << R_MIN);
	PORTC &= ~(1 << L_PLUS);
	PORTD &= ~(1 << L_MIN);
	watchdogMotor = 1;
}

void setSpeed(uint8_t speed)
{
	itoa(speed, send, 10);
	UART_Transmit_String(send);
	uint16_t newSpeed = 20000 - (20000 / 100 * speed);
	itoa(newSpeed, send, 10);
	UART_Transmit_String(send);
	OCR5A = newSpeed;
	OCR5C = newSpeed;
}
