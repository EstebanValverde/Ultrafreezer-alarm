#ifndef CARGADORGELACIDO_H
#define CARGADORGELACIDO_H
//-----------------------------------------------------------
// CARGADOR DE BATERIA DE GEL-ACIDO CON ARDUINO
//-----------------------------------------------------------

//-----------------------------------------------------------
// Pines del Arduino para carga de bateria
//-----------------------------------------------------------
#define VSENSE  A2    // canal ADC de lectura de la tension de carga
#define ISENSE  A3    // canal ADC de lectura de la corriente de carga
#define PWM_BAT 10    // pin PWM desde donde se comanda la corriente de carga
#define PWR_BAT  8    // entrada digital para saber si estoy a 220VAC o bateria

//-----------------------------------------------------------
//Variables y Constantes de Medicion de Corriente
//-----------------------------------------------------------
#define IMAX   0.5       // corriente maxima de carga [Ampere]
#define IOCT (IMAX/10.0) // corriente para entrar en modo FLOAT [Ampere]
#define IT   (IOCT/5.0)  // corriente de modo trickle hasta entrar en modo BULK (opcional) [Ampere]
#define R_SHUNT 1.014    // resistencia que mide corriente en negativo bateria [Ohm]
#define DIVISORI (144.0/303.0)  // divisor de tension en ISENSE

//-----------------------------------------------------------
//Variables y Constantes de Medicion de Tension
//-----------------------------------------------------------
#define VOC 13.56        // tension maxima de carga [Volts]
#define V12 (VOC * 0.95) // tension de modo OVERCHARGE [Volts]
#define VT  (VOC * 0.65) // tension de modo trickle hasta entrar en modo BULK (opcional) [Volts]
#define VF  13.04        // tension flotante de la bateria cargada (etapa FLOAT) [Volts]
#define DIVISORV (2.00/13.39)  // divisor de tension en VSENSE

enum {DISCHARGED = 0, BULK, OVERCHARGE, FLOAT};

class CargadorGelAcido
{
private:
	unsigned char etapa;
	unsigned int valorPWM; // valor inicial de la corriente de carga, debe ser baja
	float corriente;     // corriente de la bateria [Ampere]
	float tension;       // tension en bornes de la bateria
	float ADCvRef;
	
	void midoEstadoBateria(void);
	void ajustaPWMcargador(void);
	void downPWM(void);
	void upPWM(void)	;
public:
	CargadorGelAcido();
	~CargadorGelAcido();
	
	void begin(float);
	void procesarCargaBateria(void);
	void resetCargaBateria(void);
	
	float getCorriente(void){ return corriente; }
	float getTension(void){ return tension; }
	unsigned int getPWM(void){ return valorPWM; }
	unsigned char getEtapa(void){ return etapa; }
};

#endif
