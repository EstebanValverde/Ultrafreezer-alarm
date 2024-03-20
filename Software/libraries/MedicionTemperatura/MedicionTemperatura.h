#ifndef MedicionTemperatura_H
#define MedicionTemperatura_H

// -----------------------------------------------------------------
// configuracion del sensor de temperatura interno
// -----------------------------------------------------------------
// del termistor
#define R0 100.0 // resistencia a 0°C [ohms]
// pendiente lineal [°C-1] pero tomo ALPHAINV = 1/ALPHA = 1/0.3003908
#define ALPHAINV 255.88

// del puente
#define VR 3298.0   // tension de referencia del puente [mV]
#define R1 2176.0   // resistencia de rama superior izquierda [ohms]
#define R3 98.5548  // resistencia de rama inferior izquierda [ohms]
#define R2 2165.0   // resistencia de rama superior derecha [ohms]

// de la rama del termistor en la salida del amplificador
#define VIOA 0.0  // offset canal A0 [mV]
#define VIOB 0.0  // offset canal A1 [mV]
#define GA  3.20  // ganancia real del canal A0
#define GB  5.65  // ganancia real del canal A1

// cantidad de mediciones internas de temperatura
#define NTEMP 5

#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
#define DHTPin 5        // what digital pin we're connected to

// largo del vector del LCD
#define LCDLEN 16

class MedicionTemperatura
{
private:
	float ADCvRef;
	char temps[NTEMP];
	unsigned char iterTemp;
	char txin, txout;

public:
	MedicionTemperatura(){};
	~MedicionTemperatura(){};
	
	void begin(float);
	
	void MedirTemperaturaInterna(void); 
	void MedirTemperaturaExterna(void);
	void TemperaturaTostring(char *, char, const char*);
	
	char GetTempInterna(void){return txin;}
	char GetTempExterna(void){return txout;}
};

#endif