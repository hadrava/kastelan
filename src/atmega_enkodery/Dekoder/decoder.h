#ifndef DECODER_H
#define DECODER_H

#include <avr/io.h>

union doublebyte{
	uint16_t value;
	uint8_t bytes[2];
};

struct encoders{
	union doublebyte left;
	union doublebyte right;	
};


#endif