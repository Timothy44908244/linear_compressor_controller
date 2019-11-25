/* LED.c */
#include "led.h"

void led_init(){
	DDRC |= (1<<DDC3)|(1<<DDC4)|(1<<DDC5);	// take PC3,4,5 as output
}

void turn_on_led_1(){	// turn on LED1 at PC5
	PORTC |=(1<<PC5);
}

void turn_off_led_1(){
	PORTC &= ~(1<<PC5);
}

void turn_on_led_2(){
	PORTC |=(1<<PC4);
}

void turn_off_led_2(){
	PORTC &= ~(1<<PC4);
}

void turn_on_led_3(){
	PORTC |=(1<<PC3);
}

void turn_off_led_3(){
	PORTC &= ~(1<<PC3);
}

void toggle_led_1(){	// toggle LED1 on PC5
	PORTC ^= (1<<PC5);
}

void toggle_led_2(){
	PORTC ^= (1<<PC4);
}

void toggle_led_3(){
	PORTC ^= (1<<PC3);
}