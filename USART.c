#include "USART.h"
#include "adc.h"
#include <stdio.h>
#include <math.h>
#include <string.h>

//json package fractions
char t1[] ="\n{\"3\":{\"mfc\":{\"req\":\"";//+req
char t2[] ="\",\"cur\":\"";//+cur
char t3[] ="\"},\"ver\":\"";//+ver
char t4[] ="\",\"param\":{\"pwr\":\"";//+pwr
char t5[] ="W\",\"freq\":\"";//+freq
char t6[] ="Hz\",\"curr\":\"";//+curr
char t7[] ="mA\",\"volt\":\"";//+volt
char t8[] ="V\"},";//end of param
char t9[] ="\"clr\":\"";//+clr, start of error part
char t10[] ="\",\"ew\":";//+ew, end of error part
char t11[]="}}\n";//(end of JSON object)

void USART_Init(unsigned int ubrr)
{									// Asynchronous operation
	UBRRH = (unsigned char)(ubrr>>8);
	UBRRL = (unsigned char)ubrr;	// Set baud rate
	UCSRA = (1<<U2X);				// Double the USART transmission speed
	UCSRB = (1<<RXEN)|(1<<TXEN);	// Enable receiver and transmitter
	UCSRC = (1<<URSEL)				// Register select: UCSRC
			|(1<<UCSZ1)|(1<<UCSZ0);	// Set frame format: 8data, 1stop bit
}

void USART_transmit_byte(unsigned char data){	// transmit one byte
	while(! (UCSRA & (1<<UDRE)));				// USART Data Register Empty Flag
	UDR = data;									// put data into the buffer and send it
}

void USART_transmit(char* char_array){				// transmits a string
	uint8_t char_array_length = strlen(char_array);	// get the size of char_array
	for(uint8_t i=0;i<char_array_length;i++){		// Loop through each char
		USART_transmit_byte(char_array[i]);			// Transmit each char
	}
}

unsigned char USART_Receive(){
	while(!(UCSRA & (1<<RXC)));		// USART Receive Complete Flag
	return UDR;						// get and return received data from the buffer
}

void append(char* s, char c) {		// Append arg2 c to the end of arg1 s (note: s is an char array while c is a single char)
	uint8_t len = strlen(s);
	s[len] = c;
	s[len+1] = '\0';
}

void catstr(char* s, char* c){		// this function concatenate two strings (note: both s and c are strings)
	char token_c =  c[0];
	uint8_t len = strlen(c);
	for(uint8_t i=0;i<len;i++){
		token_c =  c[i];
		append(s, token_c);
	}
}

void setmem(char* string, int c){	// it copies c to every character of the string; write null to c to will clear the array
	for (int8_t j=0;j<=strlen(string);j++){
		string[j] = c;				// write c to byte
	}
}

void fetched(char* string){					// find two " in string and replace both of them with spaces; this function should perform every time a substring is read
	uint8_t counter2 = 0;					// only perform twice: erase a pair of double quotations
	for (int8_t k=0;k<strlen(string);k++){	// iterate through the array
		if(string[k]=='"'){					// replace if " is found ...
			counter2 ++;
			if(counter2 < 3){
				string[k]=' ';				// ... with space
			}
		}
	}
}


void catstr_broadcast(char* t_out, int error, char* req, char* cur, char* ver, char* pwr, char* freq, char* curr, char* volt, char* clr, char* ew){	//this function should transmit the fixed size & fixed order json structure
	catstr(t_out, t1);	// concatenate all the pieces
	catstr(t_out, req);
	catstr(t_out, t2);
	catstr(t_out, cur);
	catstr(t_out, t3);
	catstr(t_out, ver);
	catstr(t_out, t4);
	catstr(t_out, pwr);
	catstr(t_out, t5);
	catstr(t_out, freq);
	catstr(t_out, t6);
	catstr(t_out, curr);
	catstr(t_out, t7);
	catstr(t_out, volt);
	catstr(t_out, t8);
	if(error>0){			// skip if there is no error or warning
		catstr(t_out, t9);	// error
		catstr(t_out, clr);	// error
		catstr(t_out, t10);	// error
		catstr(t_out, ew);	// error
	}
	catstr(t_out, t11);
}
//Notes for function catstr_broadcast:
//1. append order: t1+req+t2+cur+t3+ver+t4+pwr+t5+freq+t6+curr+t7+volt+t8(+clr+t9+ew+t10)+user+t11
//2. t1~11 are fixed string and the others are inputs
//3. frequency, power and voltage values are significant to 2 decimal places;  current has 2~4 digits.
//4. units for pwr, freq, cur and volt are: W, centi Hz, mA, centi V.


int parse_json_object(char* json_object, char* req, char* clr, char* freq){	// this function parse json by fetching things between quotation marks
	uint8_t flag = 0;
	if(json_object[2]=='3'){	// ID=3 (NOTE: LCC should respond to broadcast received only when ID==3)
		flag = 1;				// turn on the flag for broadcasting after JSON received
		fetched(json_object);	// delete double quotation after ID is fetched
		char token[10];			// initialize token

		//this while loop will keep fetching the next item between next two double quotations into token (until no double quotation mark left)
		while (sscanf(json_object, "%*[^\"]\"%[^\"]\"", &token)>0){		// format: %[not"](discarded) + ["] + %[not"] + ["] + %[something](discarded)
			fetched(json_object);										// delete double quotations after key is fetched
			if (strcmp(&token, "req")==0){								// if its information about req
				if(sscanf(json_object, "%*[^\"]\"%[^\"]\"", &token)>0){	// fetch next token==req_value
					fetched(json_object);								// delete double quotations after req_value is fetched
					strcpy(req, &token);								// update req_value after information is fetched
				}
			}else if(strcmp(&token, "clr")==0){							// if its information about clr
				if(sscanf(json_object, "%*[^\"]\"%[^\"]\"", &token)>0){	// fetch next token==req_value
					fetched(json_object);								// delete double quotations after req_value is fetched
					strcpy(clr, &token);								// update clr_value after information is fetched (which is probably just "ew")
				}
			}else if(strcmp(&token, "freq")==0){						// if its information about freq
				if(sscanf(json_object, "%*[^\"]\"%[^\"]\"", &token)>0){	// fetch next token==req_value
					fetched(json_object);								// delete double quotations after req_value is fetched
					strcpy(freq, &token);								// update freq_value after information is fetched (which is probably just "ew")
				}
			}
		}
	}//else: otherwise unwanted information (e.g. "mfc")
	return flag;	// return 1 when JSON is received and parsed
	//else: if ID is not 3, ignore the JSON object.
}