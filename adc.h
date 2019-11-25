// adc
#include <avr/io.h>

void adc_init();
void process_adc(char* new_curr, char* volt, char* pwr, char* freq);
unsigned int mySqrt(unsigned long x);
void comparator_init();
uint16_t read_rms_adc(uint8_t channel);