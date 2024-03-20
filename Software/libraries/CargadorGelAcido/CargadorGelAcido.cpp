#include <CargadorGelAcido.h>
#include <Arduino.h>

CargadorGelAcido::CargadorGelAcido()
{
	etapa = DISCHARGED;
	valorPWM = 1; // valor inicial de la corriente de carga, debe ser baja
	corriente = 0.0;     // corriente de la bateria [Ampere]
	tension = 0.0;       // tension en bornes de la bateria
}

CargadorGelAcido::~CargadorGelAcido(){}

void CargadorGelAcido::begin(float ref)
{
	ADCvRef = ref;
}

//-----------------------------------------------------------
// Funcion de medicion con el sensor
//-----------------------------------------------------------
void CargadorGelAcido::procesarCargaBateria(void)
{
  // si estoy conectado a 220VAC cargo/mantengo carga de la bateria
  midoEstadoBateria();
  ajustaPWMcargador();
}
  
void CargadorGelAcido::resetCargaBateria(void)
{
  // si estoy a bateria, cuando vuelvo de 220VAC a bateria, fuerzo a modo de carga BULK
  etapa = DISCHARGED;
  valorPWM = 1; 
}

void CargadorGelAcido::midoEstadoBateria(void)
{
  float V_Shunt;
  float tensionBATGND;
  corriente = 0.0;
  tension = 0.0;
  
  unsigned char index;
  for(index = 0; index < 100; index++)
  {
    V_Shunt = static_cast<float>(analogRead(ISENSE)); // en muestras
    delayMicroseconds(10);
    tensionBATGND = static_cast<float>(analogRead(VSENSE)); // en muestras

    corriente = V_Shunt / R_SHUNT / DIVISORI * (ADCvRef / 1023.0) + corriente; // en [Ampere]
    tension = (tensionBATGND / DIVISORV - V_Shunt / DIVISORI)  * (ADCvRef / 1023.0) + tension; // en [Volts]
  }
  corriente = corriente / static_cast<float>(index);
  tension = tension / static_cast<float>(index);
}

//-----------------------------------------------------------
// Controla el PWM en funcion de la corriente de carga
//-----------------------------------------------------------
void CargadorGelAcido::ajustaPWMcargador(void)
{
  switch(etapa)
  {
    case DISCHARGED:
      // cargo a baja corriente
      if(corriente < IMAX)
      {
        if(corriente < IT)
          upPWM();
        else
          downPWM();
      }
      else
          downPWM();

      // la tension aumento a V12 y por eso paso a modo OVERCHARGE
      if(tension > VT)
        etapa = BULK;
    break;

    case BULK:
      // cargo a corriente maxima
      if(corriente < IMAX)
        upPWM();
      else
        downPWM();

      // la tension aumento a V12 y por eso paso a modo OVERCHARGE
      if(tension > V12)
        etapa = OVERCHARGE;
    break;

    case OVERCHARGE:
      // cargo a tension maxima hasta que caiga la corriente
      if(corriente < IMAX)
      {
        if(tension < VOC)
            upPWM();
        else
          downPWM();
      }
      else
          downPWM();

      // la corriente fue disminuyendo hasta IOCT y por eso paso a modo FLOAT
      if(corriente < IOCT)
        etapa = FLOAT;
    break;

    case FLOAT:
      // me quedo "flotando" en este modo hasta que se use la bateria y
      // comience nuevamente el ciclo de carga
      if(corriente < IMAX)
      {
        if(tension > VF)
          downPWM();
        else
          upPWM();
      }
      else
          downPWM();
    break;
    default: break;
  }
}

//-----------------------------------------------------------
//Funciones auxiliares
//-----------------------------------------------------------
void CargadorGelAcido::downPWM(void)
{
  if(valorPWM > 1) valorPWM--;
  delay(1);
  analogWrite(PWM_BAT, valorPWM);
}

void CargadorGelAcido::upPWM(void)
{
  if(valorPWM < 255) valorPWM++;
  delay(1);
  analogWrite(PWM_BAT, valorPWM);
}
