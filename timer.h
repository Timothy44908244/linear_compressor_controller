/* timer.h */
#ifndef TIMER_H_
#define TIMER_H_

#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdint.h>

void timer_init();
void timer_pwm(uint8_t resonant_measuring);
void mass_flow_control(char* req, char* freq);
unsigned long int myAtoi(char* c);

#endif