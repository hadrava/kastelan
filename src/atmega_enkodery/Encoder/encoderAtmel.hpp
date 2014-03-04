/*!
*  \brief     Atmega encoder reader class
*  \author    Ondrej Pilat (pilat.ondrej@seznam.cz, ondrap.cz)
*  \version   0.1
*  \date      2013
*/

#ifndef ENCODER_ATMEL_H
#define ENCODER_ATMEL_H

#include <string>
#include <unistd.h>

#include "encoder.hpp"

class encoderAtmel : public encoderReader {
	static const unsigned int BUFFER_SIZE = 5;
	static const unsigned int MAX_UINT16 = 65535;
	static const unsigned int MAX_DIFFERENCE = (MAX_UINT16 / 2);

	// I2C description for comunication
	int i2cDevice;
	Encoders encodersLastState;
	unsigned char buffer[BUFFER_SIZE];

	int dealWithEncoderOverflow(const int oldValue, const int newValue);
	int readEncodersToBuffer();
	bool checkBufferValidity(const unsigned int size);
public:
	encoderAtmel(const std::string I2CDevice = "/dev/i2c-1", const int decoderAddress = 0x30);
	unsigned int getEncodersResolution();
	Encoders getEncodersState();
	Encoders getChangeOfEncoders();

	~encoderAtmel() {
		close(i2cDevice);
	};
};


#endif