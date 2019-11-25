/* timer.c */

#include <math.h>
#include <string.h>
#include "timer.h"
#include "led.h"

extern uint8_t count1;			// import variable from main.c file
uint32_t reg_mfc = 0;
uint32_t ICR1_value = 51400;
uint8_t TCNT2_value = 0;

void timer_init() {	//initialize code
	sei();								//enable global interrupts
	TCCR1A |= (1<<WGM11);
	TCCR1B |= (1<<WGM12) | (1<<WGM13)	// turn on mode 14: fast PWM with ICR1 as TOP
			| (1<<CS10) ;				// clock selection: no prescaler
	ICR1 = 51400;						// default frequency: 12.97 Hz
	DDRB |= (1<<DDB1) | (1<<DDB2);		// take PB1 & PB2 as PWM output
}

void mass_flow_control(char* req, char* freq){	// user defined object: freq
	//freq
	reg_mfc = myAtoi(freq);
	if (reg_mfc>1050 && reg_mfc<1700){			// limit the input frequency to: 10.5Hz ~ 16Hz
		reg_mfc = 1000000000/15/reg_mfc;
		ICR1 = reg_mfc;							// change the frequency of the PWM by changing TOP value
	}

	//mfc
	reg_mfc = myAtoi(req);						// read char into int
	ICR1_value = ICR1;
	if(reg_mfc==0){								// when req = 0
		ACSR |= (1<<ACIE);						// keep the short circuit protection on
		OCR1B = 0;								// set duty cycle to zero
		OCR1A = 0;
		TCCR1A &= ~((1<<COM1A1)|(1<<COM1A0));	// disconnect A
		TCCR1A &= ~((1<<COM1B1)|(1<<COM1B0));	// disconnect B
		TIMSK &= ~(1<<TOIE1);					// overflow interrupt disable
	}else if(reg_mfc>=254){						// when req >= 255
		ACSR &= ~(1<<ACIE);						// ATTENTION: SHORT CIRCUIT PROTECTION REMOVED AT 255
		TIMSK |= (1<<TOIE1);					// overflow interrupt enable
		OCR1B = ICR1_value/10;					// update duty cycle to 90%
		OCR1A = ICR1_value/10;
	}else{										// when 0<req<255
		ACSR |= (1<<ACIE);						// keep the short circuit protection on
		TIMSK |= (1<<TOIE1);					// overflow interrupt enable
		reg_mfc = (ICR1_value * 7 / 10 - (reg_mfc * ICR1_value / 510));		// calculation of register values; it is scaled to 30~80% PWM duty cycle (i.e. req=1 correspond to 30%, req=254 correspond to 80%)
		OCR1B = reg_mfc;						// update duty cycle
		OCR1A = reg_mfc;
	}
}

void timer_pwm(uint8_t resonant_measuring) {	// this function will be accessed by interrupt to control the PWM
	if(count1 == 0){				// first quarter (only entered once)
		TCCR1A &= ~((1<<COM1A1)|(1<<COM1A0));	// disconnect A
		if (resonant_measuring==0){
		TCCR1A |= (1<<COM1B1)|(1<<COM1B0);}		// connect B	// Compare Output Mode, Fast PWM; using both channels (A & B)
	}else if (count1 == 3){			// second quarter
		TCCR1A &= ~((1<<COM1B1)|(1<<COM1B0));	// disconnect B
		TCCR1A &= ~((1<<COM1A1)|(1<<COM1A0));	// disconnect A
	}else if (count1 == 6){			// third quarter
		if (resonant_measuring==0){	//when not measuring resonant frequency, go bidirectional
		TCCR1A |= (1<<COM1A1)|(1<<COM1A0);}		// connect A
		TCCR1A &= ~((1<<COM1B1)|(1<<COM1B0));	// disconnect B
	}else if (count1 == 9){			// last quarter
		TCCR1A &= ~((1<<COM1A1)|(1<<COM1A0));	// disconnect A
		TCCR1A &= ~((1<<COM1B1)|(1<<COM1B0));	// disconnect B
	}else if (count1 == 12){		// also first quarter
		count1 = 0;					// reset counter1 to zero. end of one complete period
		if (resonant_measuring==0){
		TCCR1A |= (1<<COM1B1)|(1<<COM1B0);}		// connect B
		TCCR1A &= ~((1<<COM1A1)|(1<<COM1A0));	// disconnect A
	}
}

unsigned long int myAtoi(char* c){							// convert char array to int
	unsigned long int num = 0;
	for (uint8_t n=0;n<strlen(c);n++){						// loop through c
		if(c[n]!='\0'){
			num +=  (c[n]-'0') * pow(10, (strlen(c)-n-1));	// convert char to int byte by byte
		}
	}
	return num;
}