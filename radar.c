#define F_CPU 16000000UL //CPU freq

//libraries that will be used
#include <util/delay.h>
#include <avr/interrupt.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <avr/io.h>

#define BAUDRATE 9600 //setting the baudrate for the communication
#define BAUD_PRESCALLER (((F_CPU / (BAUDRATE * 16UL))) - 1)
char string[10]; //used for UART, will hold the string to be output
char buffer[5]; //will hold the output that is to be put into the string

int OFCounter = 0; //counter to be used with the timer

#define  Trigger_pin	PB1	//trigger pin global that will be used to allow ease of reading
//vairables that are used to calculate the sensor output
double distance;
double angle=0;
long count;

ISR(TIMER1_OVF_vect)
{
	OFCounter++; //Increments the timeroverflow counter whenever overflow occurs
}

//Delay function
void Wait()
{
	uint8_t i;
	for(i=0;i<50;i++)
	{
		_delay_loop_2(0);
		_delay_loop_2(0);
		_delay_loop_2(0);
	}

}

void USART_send( unsigned char data){ //Waits for flags, places data into data register which is sent through USART
	while(!(UCSR0A & (1<<UDRE0)));
	UDR0 = data;
	
}

void USART_putstring(char* StringPtr){ //Loops and sends out each byte
	while(*StringPtr != 0x00){
		USART_send(*StringPtr);
	    StringPtr++;
    }
	
}


void main()
{
	
	DDRB |= 0b00000010;
	
	/*==========================================================================================*/
	//setting up timer 1 to be used for the input capture
	TIMSK1 = (1 << TOIE1);    //enabling interruprs 
	TCCR1A = 0; //Clear timer register
	TCNT1 = 0; //Clear counter
	TCCR1B = 0x41; //Rising edge capture, no presclaer
	TIFR1 = 1<<ICF1; //Clear input capture flag
	TIFR1 = 1<<TOV1; //Clear Timer overflow flag

	TCCR3A|=(1<<COM3A1)|(1<<COM3B1)|(1<<WGM31);        //NON Inverted PWM
	TCCR3B|=(1<<WGM33)|(1<<WGM32)|(1<<CS31)|(1<<CS30); //PRESCALER=64 MODE 14(FAST PWM)

	ICR3=4999;  //50Hz frequency
	DDRD|=(1<<PD0);   //Set PD0 as output pin, PWM
	

	
	OCR3A=165; //initial state 
	Wait();
	
	//initalizing uart registers so that numbers can be sent to program to be output onto the window
	UBRR0H = (BAUD_PRESCALLER>>8);
	UBRR0L = (BAUD_PRESCALLER);
	UCSR0B = (1<<TXEN0); //only enable transmission since we aren't using the recieving pin.
	UCSR0C = (3<<UCSZ00); //8- bit character size
	

	while(1)
	{
		PORTB |= (1 << Trigger_pin); //activate the trigger, then turn off the trigger to send the signal
		_delay_us(10);
		PORTB &= (0 << Trigger_pin);
		
		TCCR1B = 0x41;		//rising edge capture 
		TCNT1 = 0;			//clear the counter for timer1
		TIFR1 = 1<<ICF1;		//clear input capture flag
		TIFR1 = 1<<TOV1;		//clear timer overflow

		while ((TIFR1 & (1 << ICF1)) == 0);    //Wait until rising edge is seen
		TCNT1 = 0;            // clear timer 1 counter again
		TCCR1B = 0x01;        //falling edge capture
		TIFR1 = 1<<ICF1;        //clear input capture 
		TIFR1 = 1<<TOV1;        //clear overflow flag
		OFCounter = 0;    // overflow counter cleared and reset

		while ((TIFR1 & (1 << ICF1)) == 0); //Wwait until falling edge is seen
		count = ICR1 + (65535 * OFCounter);    //use capture register value for calculation
		distance = (double)count / (58*16);    //calculate the distance from the result of the caluclation

		itoa(angle, buffer, 10); //convert angle to string and into buffer
		dtostrf(distance, 2, 0, string);//convert distance to string to be put into USART

		//loading strings into buffer to format for the USART output.
		strcat(buffer, ","); 
		strcat(buffer, string);
		strcat(buffer, ".");
		USART_putstring(buffer);    /* Print distance on Terminal */
		
		OCR3A += 5; //adjust each angle by 10
		Wait();
		
		angle = (OCR3A -165) / 2.5; //calculate angle value with each iteration of OCR3A
		_delay_ms(500);
		
		//once OCR3A is at max angle, reset everything to recalculate and reset entire motor.
		if (OCR3A >= 626){
			main();	
		}
		else{
		}
		
	}
	return 0;
}
