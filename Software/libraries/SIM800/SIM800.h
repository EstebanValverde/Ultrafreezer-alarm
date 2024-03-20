#ifndef SIM800_H
#define SIM800_H

#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>

#define CELU_1 "+549xxxxxxxxxx" // cell number #1
#define CELU_2 "+549xxxxxxxxxx" // cell number #2
#define CELU_3 "+549xxxxxxxxxx" // cell number #3

#define CF(x) (reinterpret_cast<const __FlashStringHelper *>(x))
#define FC(x) (reinterpret_cast<const char *>(x))

class SIM800 : public SoftwareSerial
{
private:
	LiquidCrystal_I2C *lcd;

public:
	SIM800(int, int);
	~SIM800();
	
	void setLcd(LiquidCrystal_I2C *);
	
	// enviar mensajes que estan en la FLASH
	// void sendMessageSMS(const __FlashStringHelper *);
	// enviar mensajes que estan en la RAM
	void sendMessageSMS(const char *);
	
	unsigned char status(const char *);
};

#endif
