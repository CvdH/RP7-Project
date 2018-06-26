#define BAUDRATE 9600

#define F_CPU 16000000
#include <avr/io.h>
#include <stdlib.h>
#include <util/delay.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

void initUsart0();
void writeString(char* s);
void writeInt(int16_t nr);
void writeULong(unsigned long nr);

static void TaakHoofdletter(void *pvParameters);
static void TaakKleineletter(void *pvParameters);
SemaphoreHandle_t sem;

void wacht(unsigned long*);

TaskHandle_t hoofdHandle = NULL;
TaskHandle_t kleineHandle = NULL;
unsigned long waarde=0;

void vApplicationIdleHook( void ) {
	DDRL = 1 << PL2;  //pin 47
	
	while(1) {
		writeULong(waarde);
		writeString("\n\r");
		for(int x=0;x<20;x++) {
		     PORTL ^= (1 << PL2);
	         _delay_ms(1000);
		}
	}	
}


int main(void)
{
	sem = xSemaphoreCreateBinary();
	xSemaphoreGive(sem);
   initUsart0();
	xTaskCreate(
	TaakHoofdletter
	,  (const portCHAR *)"HoofdL" 
	,  256	                           
	,  NULL
	,  3
	,  &hoofdHandle ); // */

	xTaskCreate(
	TaakKleineletter
	,  (const portCHAR *)"KleinL" 
	,  256	
	,  NULL
	,  3
	,  &kleineHandle ); // */

	vTaskStartScheduler();

}

static void TaakHoofdletter(void *pvParameters) 
{
 TickType_t xLastWakeTime;
 const TickType_t xFrequency = 1000;

 xLastWakeTime = xTaskGetTickCount();
 
    while(1)
    {	
		for(unsigned long i=0;i<10;i++){
			writeString("ABCDEFGHIJKLMNOPQRSTUVWXYZ");
		}
        vTaskDelayUntil( &xLastWakeTime, xFrequency );

    }
}

static void TaakKleineletter(void *pvParameters) 
{
 TickType_t xLastWakeTime;
 const TickType_t xFrequency = 1000;

 xLastWakeTime = xTaskGetTickCount();


	while(1)
	{
		for(unsigned long i=0;i<10;i++){
			writeString("abcdefghijklmnopqrstuvwxyz");
		}
				
        vTaskDelayUntil( &xLastWakeTime, xFrequency );
	}
}


void initUsart0() {
	
		UCSR0A = 0;
		UCSR0B = (1 << TXEN0) |(1<<RXEN0);//  | (1 << RXCIE0);
		UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);// | (1 << UPM01);
		UBRR0 = F_CPU/((long)16 * BAUDRATE) -1;
}

void writeChar(char c)
{
	while(!(UCSR0A & (1<<UDRE0)));
	UDR0=c;
}

void writeString(char* s)
{
	xSemaphoreTake(sem, portMAX_DELAY);
	while(*s)
		writeChar(*s++);
	xSemaphoreGive(sem);
}

void writeInt(int16_t nr)
{
	char buffer[8];
	itoa(nr,buffer,10);
	writeString(buffer);
}

void writeULong(unsigned long nr)
{
	char buffer[24];
	ultoa(nr,buffer,10);
	writeString(buffer);
}
