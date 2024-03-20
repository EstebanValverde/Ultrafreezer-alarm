#include <MedicionTemperatura.h>
#include "DHT.h"
#include "QuickMedianLib.h"

DHT dht(DHTPin, DHTTYPE);

// -----------------------------------------------------------------
// funciones de la medicion y calculo de la temperatura
// -----------------------------------------------------------------
void MedicionTemperatura::begin(float ref)
{
  // referencia de tension del conversor ADC
  ADCvRef = ref; 
  
  // inicializo temperatura externa
  dht.begin(); 

  // temperaturas medibles
  txin = 0;
  txout = 0;

  // lleno el vector de temperaturas con valores validos
  iterTemp = 0;
  for(unsigned char i=0; i<NTEMP; i++)
	MedirTemperaturaInterna();
}

// -----------------------------------------------------------------
void MedicionTemperatura::MedirTemperaturaInterna(void)  
{
  // mido la temperatura interna del freezer con el sensor HEL-717
  // resolucion INTERNAL es ADCvRef/1023bits = 4mV/bit
  volatile float VA = analogRead(A0) * (ADCvRef * 1000.0 / 1023.0 / GA); // en [mV]
  VA = VA - VIOA / GA;
  delay(10);
  volatile float VB = analogRead(A1) * (ADCvRef * 1000.0 / 1023.0 / GB); // en [mV]
  VB = VB - VIOB / GB;

  // usando ambas ramas del puente
  volatile float M = R3/(R1+R3) - (VA-VB)/VR;
  // usando la rama derecha del puente
  //float M = VB / VR;

  volatile float Rx = R2 * M / (1.0 - M);
  
  // la temperatura la dejo en tipo char (-128 a +127)
  char tx = static_cast<char>((Rx/R0 - 1.0) * ALPHAINV);

  // almaceno en un buffer circular las ultimas 5 lecturas
  // cosa de tener la mediana y una lectura mas estable en
  // el display
  temps[iterTemp++] = tx;
  iterTemp = (iterTemp) % NTEMP;
  
  // calculo la mediana de 5 valores para evitar fluctuaciones
  // en la medicion de la temperatura. Es equivalente a tener el
  // promedio de las ultimas 5 mediciones
  txin = QuickMedian<char>::GetMedian(temps, NTEMP);
}

// -----------------------------------------------------------------
void MedicionTemperatura::MedirTemperaturaExterna(void)  
{
  // mido la temepratura ambiente en el DHT22
  txout = static_cast<char>(dht.readTemperature());
}

// -----------------------------------------------------------------
void MedicionTemperatura::TemperaturaTostring(char *strlcd, char tx, const char* str)
{
  //char strtx[20];
  
  // limito para que entre todo en una linea del LCD
  if(tx > 99)  tx =  99;
  if(tx < -99) tx = -99;

  // armo el mensaje a mostrar en el LCD
  sprintf(strlcd, "%s %d*C", str, tx);

  // fuerzo a terminar en LCDLEN en caso que el string sea mas grande
  strlcd[LCDLEN] = '\0'; 
}
