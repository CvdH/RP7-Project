/*
 * sonarsensort.cpp
 *
 * Created: 5/25/2018 11:36:00 AM
 * Author : Tulp
 */
 
 
 
 
  #ifndef F_CPU
  #define F_CPU 16000000UL
  #endif
  
  #define TRIGGER PB0

  #include <avr/io.h>
  #include <util/delay.h>
  #include <avr/interrupt.h>
  #include <avr/wdt.h>

  #include <stdio.h>
  #include <stdlib.h>
  #include <stdint.h>

  #define BAUD 9600
  #define MYUBRR (((F_CPU / (16UL * BAUD))) - 1)

  #define RECEIVED_TRUE 1
  #define RECEIVED_FALSE 0

  #define INSTR_PER_US 16												// instructions per microsecond (depends on MCU clock, 16MHz current)
  #define INSTR_PER_MS 16000											// instructions per millisecond (depends on MCU clock, 16MHz current)
  #define MAX_RESP_TIME_MS 200										// timeout - max time to wait for low voltage drop
  #define DELAY_BETWEEN_TESTS_MS 50									// echo cancelling time between sampling

  void wait(unsigned int);
  void UART_Init( unsigned int ubrr );
  unsigned char UART_Receive( void );
  void UART_Transmit( unsigned char data );
  void UART_Transmit_String(const char *stringPtr);

  void INT1_init( void );
  void pulse( void );
  void timer3_init( void );

  
  void turnServo(int degrees);
  void initServo();
  void addToBuffer(char x);

  char buffer[3];
  int index;

  char ontvang;														// Global variable to store received data
  int state = RECEIVED_FALSE;
  char out[256];

  volatile uint8_t running = 0;										// State to see if the pulse has been sent.
  volatile uint8_t up = 0;
  volatile uint32_t timerCounter = 0;
  volatile unsigned long result = 0;
  
  //echo = digital 20
  //trigger = digital 53
  //servo = digital 12


  int main()
  {
	  DDRB = (1 << TRIGGER);											// Trigger pin

	  UART_Init(MYUBRR);
	  INT1_init();
	  initServo();
	  wdt_enable(WDTO_2S);
	  timer3_init();
	  sei();

	  // enable watchdog timer at 2 seconds
	  UART_Transmit_String("Setup done");
	  while(1)
	  {
		  if(running == 0)
		  {
			  _delay_ms(DELAY_BETWEEN_TESTS_MS);
			  pulse();
			  sprintf(out, "Afstand = %dCM", result);
			  UART_Transmit_String(out);
			  
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
		  index=0;
		  }else{
		  buffer[index++]=x;
	  }
	  //UART_Transmit(x);
  }

  void wait(unsigned int a)
  {
	  while(a--)
	  _delay_ms(50);
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

  unsigned char UART_Receive( void )
  {
	  while ( !(UCSR0A & (1<<RXC0)) )	{};								// Wait for data to be received
	  return UDR0;													// Get and return received data from buffer
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
	  if(running)														// check if the pulse has been sent
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
			  if(result != 0)
				wdt_reset();
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
	  TCCR3B |= (0 << CS30) | (1 << CS31) | (0 << CS32);				// prescaler 0
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
 
