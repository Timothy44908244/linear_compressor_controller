#include <avr/io.h>

#ifndef USART_H_
#define USART_H_

void USART_Init(unsigned int ubrr);
void USART_transmit_byte(unsigned char data);
unsigned char USART_Receive();
void USART_transmit(char* char_array);
void append(char* s, char c);
void catstr(char* s, char* c);
void catstr_broadcast(char* t_out, int error, char* req, char* cur, char* ver, char* pwr, char* freq, char* curr, char* volt, char* clr, char* ew);// note: expensive
void setmem();
void fetched(char* string);	// note: each time cannot fetch more than four times (reason unknown)
int parse_json_object(char* json_object, char* req, char* clr, char* freq);

#endif