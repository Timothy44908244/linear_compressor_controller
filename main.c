/*
 * test_timer.c
 * Created: 28/08/2019 5:16:16 PM
 * Author : tzha384
 */

//defines
#define F_CPU 8000000UL			// clock frequency: 8MHz
#define FOSC 8000000			// Clock Speed
#define BAUD 9600				// Baud Rate set
#define MY_UBRR FOSC/8/BAUD-1	// UBRR value, with prescalar of 8 according to fast PWM double speed mode

//includes
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h> // more portable code by providing a set of typedefs that specify exact-width integer types, together with the defined minimum and maximum allowable values for each type, using macros
#include <avr/io.h>
#include <util/delay.h>
#include "timer.h"
#include "led.h"
#include "adc.h"
#include "USART.h"

//flags
uint8_t broadcast_flag = 0;							// flag that indicates the start of broadcasting full package to PC
volatile static uint8_t timer_pwm_flag = 0;			// volatile flag for ISR to count period
uint8_t json_receiving_flag = 0;					// flag indicating the start of receiving json
uint8_t collision_protection = 0;					// flag that indicates if the collision protection feature (beta) has been turned on
uint8_t max_to_min = 0;								// flag for resonant frequency detection(feature not implemented) to detect max value and min value
uint8_t error_broadcast = 0;						// flag of error reporting
uint8_t resonant_measuring = 0;						// flag for pausing PWM and measure resonant frequency

//counters
uint8_t counter_left_bracket = 0;	// counting numbers of left curly brackets in json package received
uint8_t counter_right_bracket = 0;	// counting numbers of right curly brackets in json package received
volatile uint8_t count1 = 0;		// counter for timer1 to help shape the PWM driving signal
uint16_t ovf_count = 0;				// counter for timer1 overflow

//variables
uint8_t received_byte;			// last byte received from comm line
char json_object_buffer[255];	// json package received/ready to be transmitted (both share the same buffer)

// variables
uint8_t error_state = 0;		// write this to zero will clear value of clr & ew until they are given new values; 1:short; 2:collision
char req[4]="";					// mass flow control value
char cur[4]="0";				// last req value
char ver[12]="2.4.4";			// V2.4.4	updated on 19/10/2019	//beta version with new collision detection feature
char pwr[6]="";					// RMS system power
char freq[6]="";				// input PWM frequency
char curr[4]="";				// RMS current
char volt[6]="";				// average Vcc voltage
char clr[3]="";					// clr value
char ew[20]="";					// error message
uint8_t req_mfc = 0;			// the integer of the req value (0~255)

// variable for testing
uint16_t test_int = 0;			// variable for testing resonant frequency measuring (not implemented)
uint16_t this_measure = 13;		// this adc reading, for collision protection (beta)
uint16_t last_measure = 23;		// last adc reading, for collision protection (beta)
int16_t this_difference = 33;	// the difference between last two adc readings, for collision protection (beta)
int16_t last_difference = 43;	// last difference between last two adc readings, for collision protection (beta)
uint32_t ICR1_val = 0;			// value of ICR1 for testing resonant frequency measuring (not implemented)
char msg[8];					// char array for multiple printing/testing purposes

//main
int main(void) {
	comparator_init();		// initialize short circuit protection
	led_init();				// initialize LEDs
	timer_init();			// initialize timer
	USART_Init(MY_UBRR);	// initialize transceiver
	adc_init();				// initial adc

	while (1) {		// infinity loop

		if(collision_protection==1){								// when collision protection feature is on
			turn_on_led_3();										// use LED3 to indicate this feature on/off state
			if((count1>3 && count1<6)||(count1>9 && count1<12)){	// when both PWM channels are disconnected
				last_measure = this_measure;						// update old adc measurement
				this_measure = read_rms_adc(2);						// read the adc and record the value
				last_difference = this_difference;					// update old derivative value
				this_difference = this_measure - last_measure;		// update current derivative value
				_delay_ms(6.6);										// this delay is to slow the ADC further down. this number is specially tunned for the current system
				if ((this_difference+last_difference)>-2 && (this_difference+last_difference)<2){	//if the derivative is smaller than 2
					error_state=2;									// error flag: on
					error_broadcast=1;								// broadcast the error
					last_measure=11;								// clear the value
					this_measure=73;
				}
			}
		}else{
			turn_off_led_3();
		}

		if(UCSRA&(1<<RXC)){										// check buffer and receive
			received_byte = USART_Receive();					// receive in polling
			if(json_receiving_flag==0){							// enter state: NORMAL RECEIVING MODE
				turn_off_led_2();								// use LED2 to indicate receiving/idle
				if(received_byte == '{'){						// detect '{' to start "receiving json" state
					counter_left_bracket++;						// initialize left&right bracket counters
					append(&json_object_buffer, received_byte);	// manually load the very first byte
					json_receiving_flag = 1;					// flag of start receiving json
					turn_on_led_2();							// use LED2 to indicate receiving/idle
				}

				//(START OF TESTING CODE: resonant frequency)
				else if (received_byte == '?')	// start testing by input '=' in terminal
				{
					USART_transmit_byte('<');
					ICR1_val = (ICR1>>1);		// read TOP value from ICR1
					ICR1_val = (ICR1_val<<1);
					uint32_t resonant_time=0;	// initialize variables
					max_to_min=0;				// first detect max instead of min
					while(count1!=1);			// "right" after the PWM is off
					resonant_measuring=1;		// shut down the PWM for one period
					while(count1!=11){			// enter resonant frequency calculating state
						last_measure = this_measure;
						test_int = read_rms_adc(2);
						this_measure = test_int;
						if(this_measure<=last_measure && max_to_min==0){	//max detected, df/dt=0; start timer
							ovf_count=0;							// number of overflows reset to zero
							resonant_time = (TCNT1>>6);				// start timer and record counter number into resonant_time
							resonant_time = (resonant_time<<6);		// UNKNOWN ERROR: TCNT1 is overflowing uint32_t
							max_to_min=1;							// now detect min instead of max
						}
						if(this_measure>=last_measure && max_to_min==1){	//min detected, df/dt=0; stop timer; this is triggered only enter after max has been detected
							uint32_t stop_time = (TCNT1>>6);		//stop timer
							stop_time = (stop_time<<6);				// UNKNOWN ERROR: TCNT1 reading error
							max_to_min=2;							// stop detection of either max or min
							resonant_time = ovf_count * ICR1_val - resonant_time + stop_time;	// calculation total clock cycles between start of timer and stop of timer
						}
				}//end of "while(count1!10)"						//exit resonant frequency calculating state
				if (max_to_min==2){									// if both min and max have been successfully captured
					resonant_time=400000000/resonant_time;			// and here the first resonant_time calculated as the frequency in centi unit (reuse variable to save memory)
					itoa(resonant_time, msg, 10);					// int to array
					USART_transmit(msg);							// print on terminal
				}
				resonant_measuring=0;								// turn back on the PWM and continue normal operation
				USART_transmit_byte('>');
				received_byte=' ';
			}//(END OF TESTING CODE: resonant frequency)

			//(START OF TESTING CODE: short circuit protection)
			else if (received_byte == '$')								// testing short by enter 's' as input in terminal. the related interrupt will be forced to trigger
			{
				ANA_COMP_vect();
				received_byte=' ';
			}//(END OF TESTING CODE: short circuit protection)

			//(START OF TESTING CODE: collision protection)
			else if(received_byte == '#'){								// testing collision
				collision_protection = !collision_protection;
				received_byte=' ';
			}//(END OF TESTING CODE: collision protection)

			}else{														// JSON RECEIVING MODE
				if(received_byte == '{'){counter_left_bracket++;}		// counter: left bracket
				else if(received_byte == '}'){counter_right_bracket++;}	// counter: right bracket
				if(received_byte!=' '){									// filter out spaces
					append(&json_object_buffer, received_byte);			// append the received bytes into the buffer
				}
				if(counter_left_bracket==counter_right_bracket){		// only when number of left brackets and number of right brackets match, JSON package is received completely. ready to exit state
					json_receiving_flag = 0;							// exit JSON receiving mode
					counter_left_bracket = 0;							// clear counter
					counter_right_bracket = 0;							// clear counter
					broadcast_flag = parse_json_object(&json_object_buffer, &req, &clr, &freq);	// parse the JSON and update values of req, clr. and then turn on flag for sending the broadcast
					if(strcmp(&clr,"ew")==0){	// if a clear command is received, clear the error and reset LED1
						error_state=0;			// clear warning
						error_broadcast=0;		// stop sending error information
						turn_off_led_1();
						ew[0]='\0';
					}
					mass_flow_control(req, freq);					// write the new values into the register to change the duty cycle/frequency
					req_mfc = myAtoi(req);							// convert it into int for future comparing
					setmem(&json_object_buffer, '\0');				// clear the buffer after JSON parsing and be ready to receive a new object
				}
				if(received_byte == '|'){json_receiving_flag=0;}	// send pipe to force trigger normal receiving mode (only use when package corrupted)
			}														//end of JSON receiving mode
		}
		if (broadcast_flag == 1){									// when the flag is turned on after receiving json with its ID==3
			process_adc(curr, volt, pwr, freq);						// calculate rms current/voltage/power and clear the measuring
			catstr_broadcast(&json_object_buffer,error_state,&req,&cur,&ver,&pwr,&freq,&curr,&volt,&clr,&ew);	// modify the JSON package to broadcast
			UCSRB &= ~(1<<RXEN);
			USART_transmit(json_object_buffer);						// send the full broadcast to PC
			UCSRB |= (1<<RXEN);
			strcpy(cur, req);
			setmem(&json_object_buffer, '\0');						// clear the buffer after broadcasting
			broadcast_flag = 0;
		}
		if(error_broadcast == 1){						// broadcast the error when there is a warning
			if(error_state==1){							// 1: SHORT CIRCUIT PROTECTION
				TCCR1A &= ~((1<<COM1A1)|(1<<COM1A0));	// disconnect PWM channel A
				TCCR1A &= ~((1<<COM1B1)|(1<<COM1B0));	// disconnect PWM channel B
				TIMSK &= ~(1<<TOIE1);					// disable timer1 interrupt
				OCR1B = 0;								// update duty cycle to 0%
				OCR1A = 0;
				turn_on_led_1();						// turn on red LED1
				turn_on_led_2();						// turn on LED2 too
				strcpy(ew, "\"shortCircuit\"");			// load the content of the warning
				broadcast_flag=1;						// broadcast the error
				error_broadcast=0;						// exit error broadcast state (to only broadcast error once)
				strcpy(req, "000");						// set req to 0
			}else if (error_state==2){					// 2: COLLISION DETECTION
				TCCR1A &= ~((1<<COM1A1)|(1<<COM1A0));	// disconnect A
				TCCR1A &= ~((1<<COM1B1)|(1<<COM1B0));	// disconnect B
				TIMSK &= ~(1<<TOIE1);					// disable timer1 interrupt
				OCR1B = 0;								// update duty cycle to 0%
				OCR1A = 0;
				turn_on_led_1();						// turn on red LED1
				turn_off_led_2();						// turn off LED2
				strcpy(ew, "\"pistonCollision\"");		// load the content of the warning
				broadcast_flag=1;
				error_broadcast=0;
				collision_protection=0;					// after the collision detected, turn off this beta functionality
				strcpy(req, "000");						// set req to 0
			}
		}
	}//end of infinity loop while(1)
}

ISR(TIMER1_OVF_vect){				// interrupt to produce PWM driving signals
	timer_pwm(resonant_measuring);	// call function to turn on/off A/B PWM channels
	count1++;						// set counter
	ovf_count++;					// count the overflows (for testing resonant frequency)
}


ISR(ANA_COMP_vect){					// interrupt to detect short circuit current using analogue comparator
	error_state=1;					// set warning flag
	error_broadcast=1;				// broadcast warning
}