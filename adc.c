// ADC
#include "adc.h"
#include "timer.h"
#include "USART.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>

char adc_char[10] = "";		//10 for the length of uint32_t
uint32_t v_val=0;			//value of voltage
uint16_t adc_reading=0;
uint32_t time_period=0;		//time period for frequency calculating
uint32_t adc_sum = 0;		//sum of adc_reading squares
uint16_t num_sample = 0;	//counter for number of adc samples

void comparator_init(){
	ACSR |= (1<<ACIE);		//Analog Comparator Interrupt Enable
}

void adc_init(){
	ADCSRA |= (1<<ADEN)							// adc enable
		     |(1<<ADPS0)|(1<<ADPS1)|(1<<ADPS2);	// prescaler 128
	ADMUX |=  (1<<REFS0);						// using 5V AVCC with external capacitor at AREF pin for the voltage reference
	DDRC &= ~ ((1<<PC0)|(1<<PC1)|(1<<PC2));		// take PC0,1,2 as adc inputs
	PORTC = 0x00;								// clear port C
}

void process_adc(char* new_curr, char* volt, char* pwr, char* freq){
//freq
	time_period = ICR1;								// get TOP value for next step calculation
	time_period = 1000000000 / 15 / time_period;	// (10^9)/(1500*ICR1) = 1/T = f. its the frequency here. unit:centi
	itoa(time_period, adc_char, 10);
	adc_char[4] = adc_char[3];
	adc_char[3] = adc_char[2];
	adc_char[2] = '.';								// add a decimal point
	adc_char[5] = '\0';
	strcpy(freq, &adc_char);

//volt
	num_sample = 256;
	adc_sum=0;
	while(num_sample>0){										// take 256 samples in this loop
		adc_reading = read_rms_adc(0);
		adc_sum += adc_reading;
		num_sample--;
	}
	adc_sum = (adc_sum >> 8);									// divide by 256
	v_val = (uint32_t)adc_sum * (uint32_t)180664/10000 + 700;	// unit: milli volt	// compensate 0.7V across diode
	itoa(v_val, &adc_char, 10);
	adc_char[4] = adc_char[3];
	adc_char[3] = adc_char[2];
	if(adc_char[0]=='1'){										// for voltage above 10V
		adc_char[2] = '.';
	}else{														// for voltage under 10V
		adc_char[3] = adc_char[1];
		adc_char[1] = '.';
	}
	adc_char[5] = '\0';
	strcpy(volt, &adc_char);

//curr
	num_sample = 4096;
	adc_sum = 0;
	while(num_sample>0){						// take 4096 samples
		adc_reading = read_rms_adc(1);
		adc_sum += adc_reading * adc_reading;	// append its square value to the sum
		num_sample--;
	}
	adc_sum = (adc_sum >> 12);					// divide by 4096
	adc_sum = sqrt(adc_sum);					// square root
	adc_sum = adc_sum * 87193/10000;			// difference amplifier conversion ratio (raw_adc->amp)
	itoa(adc_sum, &adc_char, 10);
	strcpy(new_curr, &adc_char);

//pwr
	v_val = (uint32_t)adc_sum * (v_val/1000);	// power = the rms_curr multiply the volt
	itoa(v_val, adc_char, 10);
	adc_char[4] = adc_char[3];
	adc_char[3] = adc_char[2];
	if(adc_char[0]=='1'){
		adc_char[2] = '.';
	}else if (adc_char[0]=='0'){

	}else{
		adc_char[2] = adc_char[1];
		adc_char[1] = '.';
	}
	adc_char[5] = '\0';
	strcpy(pwr, &adc_char);
}

uint16_t read_rms_adc(uint8_t channel){		// curr channel: channel = 1	// volt channel: channel = 0
	ADMUX&=0xF0;
	ADMUX|=channel;							// adc channel selection
	ADCSRA|=(1<<ADSC);
	while((ADCSRA&(1<<ADIF))==0);
	adc_reading = ADCW;
	return adc_reading;
}