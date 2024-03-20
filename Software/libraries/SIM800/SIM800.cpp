#include <SIM800.h>
#include <Arduino.h>

// #define DEBUGSIM800

// formato AT+CMGS="+54911xxxxxxxx" <-- 24 caracteres de longitud
// const char celulares[][30] = {
const char celulares[][30] PROGMEM = { \
	"AT+CMGS=\"" CELU_1 "\"", \
	"AT+CMGS=\"" CELU_2 "\"", \
	"AT+CMGS=\"" CELU_3 "\""};

#define CELL_NUMBERS 3

// -----------------------------------------------------------------
// funciones del SIM800
// -----------------------------------------------------------------
SIM800::SIM800(int rxPin, int txPin):SoftwareSerial(rxPin, txPin)
{
	lcd = NULL;
}

SIM800::~SIM800(){}

void SIM800::setLcd(LiquidCrystal_I2C *LCD)
{
	lcd = LCD;
}

// void SIM800::sendMessageSMS(const __FlashStringHelper *str)
// {
	// char aux[50];
	// strcpy_P(aux, FC(str));
	// sendMessageSMS(aux);
// }

void SIM800::sendMessageSMS(const char *str)
{
	for(unsigned char i=0; i<CELL_NUMBERS; i++)
	{
		// envio mensajes a varios numeros de telefono
		// leo cualquier cosa que haya quedado dentro de la SIM
		while(available() != 0) read();	

		// envio un mensaje SMS
		println(F("AT"));
		status(NULL);

		// Show ICCID, muestra el codigo de la SIM
		println(F("AT+CCID")); 
		status(NULL);

		// should return "READY"
		println(F("AT+CPIN?")); 
		status(NULL);

		// Signal Quality Report debe ser mayor a "5"
		println(F("AT+CSQ")); 
		status(NULL);

		// checks module state, should be "1" for full functionality
		println(F("AT+CFUN?")); 
		status(NULL);

		// Operator Selection: 0 Unknown, 1 Operator available, 2 Operator current, 3 Operator forbidden
		println(F("AT+COPS?")); 
		status(NULL);

		// Network Registration: 0 Not registered, 1 Registered, 2 Not registered, 
		// 3 Registration denied, 4 Unknown, 5 roaming 
		if(lcd)
		{
			lcd->clear();
			lcd->print(F("Buscando Red..."));
		}

		println(F("AT+CREG?")); 
		while(status("CREG") == 0) // 1 Registered
		{
			println(F("AT+CREG?")); 
		}

		// Envio SMS de bienvenida. Si se recibe es porque el sistema esta online
		if(lcd)
		{
			lcd->clear();
			lcd->print(F("Enviando SMS..."));
		}

		// Select SMS Message Format text mode
		println(F("AT+CMGF=1")); 
		status(NULL);

		// change ZZ with country code and xxxxxxxxxxx with phone number to SMS
		println(CF(celulares[i])); 
		status(NULL);

		// envio el mensaje por SMS
		println(str);
		status(NULL);

		// CTRL+Z para la SIM, significa fin del mensaje y terminar
		write(26);
		status(NULL);

		// para que se recupere el sistema del consumo de corriente ???
		delay(10000);
	}
}

unsigned char SIM800::status(const char *str)
{
  // leo la respuesta del SIM800 al comando AT que se le envio justo recien. 
  char inchar;
  unsigned char flag = 0, i = 0;

  // Devuelve el mismo comando AT con la respuesta
  // a dicho comando en una cadena de caracteres. Leo de a uno
  // por vez y lo voy almacenando en un string para mostrar por LCD
  // o enviar al monitor serie de la PC. 
  // flush();
  while(available() != 0)  
  { 
    inchar = read();
    // este código es para verificar si el comando devuelto por el SIM
    // es el mismo que envié, por ejemplo CREG. Esto pasa si se ejecuta
    // el comando y entonces se entiende que fue OK
    if(str != NULL)
    {
      // no considero caracteres que no sean alfanumericos basicos
      if(inchar >= ' ' && inchar <= 'z')
      {
        // por cada caracter leido desde la SIM compar con la secuencia
        // de caracteres de "str". Si está completa y en orden, entonces
        // el valor de "i" sera igual al largo de "str". Caso contrario, 
        // que haya al menos un caracter diferente, entonces i=0 y no
        if(inchar == str[i])
        {
          i++;
          if(i == strlen(str)) flag = 1;
        }
        else i = 0;
      }
    }

	#ifdef DEBUGSIM800
	if(inchar >= ' ' && inchar <= 'z')
		Serial.print(inchar);
	#endif
  }
  
  #ifdef DEBUGSIM800
	Serial.println(" ");
  #endif
  
  delay(750);
  return flag;
}

