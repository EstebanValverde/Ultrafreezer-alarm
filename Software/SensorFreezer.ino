#include <Flag.h>
#include <SIM800.h>
#include <CargadorGelAcido.h>
#include <MedicionTemperatura.h>

// -----------------------------------------------------------------
// diferentes opciones de debuggear enviando datos por puerto serie
// -----------------------------------------------------------------
// #define DEBUGSIM800  // se debe definir en SIM800.cpp en la carpeta de bibliotecas
// #define DEBUGBAT
// #define DEBUGTEMP

// -----------------------------------------------------------------
// objeto que mide temperatura interna y externa
// configuracion del sensor de temperatura externo ver en el .h
// -----------------------------------------------------------------
#define TEMPINTALARM -55
#define TEMPEXTALARM  40
MedicionTemperatura temp;
unsigned char countTinAlarm = 0;
unsigned char countTexAlarm = 0;

// -----------------------------------------------------------------
// Create software serial object to communicate with SIM800
// CHIP MOVISTAR 11-3661-3941
// -----------------------------------------------------------------
SIM800 mySim(3, 2); //SIM800 Rx & Tx is connected to Arduino #2 & #3
// define en el .h los pines utilizados por esta biblioteca
// tambien se definen los numeros de telefono que reciben los mensajes

//-----------------------------------------------------------
// Pines del Arduino para carga de bateria
//-----------------------------------------------------------
CargadorGelAcido Bat; // define en el .h los pines utilizados por esta biblioteca
Flag flagBat;

// -----------------------------------------------------------------
// Crear el objeto lcd  direccion  0x3F y 16 columnas x 2 filas
// -----------------------------------------------------------------
LiquidCrystal_I2C lcd(0x27, LCDLEN, 2); // [cols, rows]
char strTempInt[20];
char strTempExt[20];
char strTemps[40];
char strOut[64];

// -----------------------------------------------------------------
// cadenas de caracteres a display y salida serie
// -----------------------------------------------------------------
const char strSistemaBat[]         = "Sistema Bateria";
const char strSistema220[]         = "Sistema Red 220";
const char strAlarmaTemperatura[]  = "Alarma Temperatura";
const char strSistemaOnline[]      = "Sistema Online";

const char lcdAlarmaFreezer[]      = "Alarma Freezer";
const char lcdAlarmaAmbiente[]     = "Alarma Room";

//-----------------------------------------------------------
// Referencia del conversor ADC
//-----------------------------------------------------------
#define ADCREF50	_BV(REFS0) // referencia interna de 5.0 Volts, REFS1 en "0" y REFS0 en "1"
#define ADCREF11	(_BV(REFS0) | _BV(REFS1)) // referencia interna de 1.1 Volts, REFS1 en "1" y REFS0 en "1"
volatile float ADCvRef = 0.0; // referencia del tension del ADC (5.0V o 1.1V)

// -----------------------------------------------------------------
// conteo de tiempo usando millis(). cuenta la cantidad de milisegundos
// desde que se caraga el programa, o se resetea la placa arduino.
// para reiniciar, hay un codigo por ahi, pero puede enviarse un jmp(0x00)
// reiniciar el programa.
// -----------------------------------------------------------------
// millis() cuenta hasta 2^32 =        4.294.967.296
#define UNSEGUNDO 1000UL            //         1.000
#define UNMINUTO (UNSEGUNDO * 60UL) //         6.000
#define UNAHORA (UNMINUTO * 60UL)   //     3.600.000
#define UNDIA (UNAHORA * 24UL)      //    86.400.000
#define UNASEMANA (UNDIA * 7UL)     //   604.800.000
#define UNMES (UNDIA * 30UL)        // 2.592.000.000
volatile unsigned long actual = 0;
volatile unsigned long timeTinCounter = 0;
volatile unsigned long timeTexCounter = 0;

// -----------------------------------------------------------------
// funciones generales del arduino
// -----------------------------------------------------------------
void setup()
{
  // Begin serial communication with Arduino and PC
  #if defined DEBUGSIM800 || defined DEBUGBAT || defined DEBUGTEMP
    Serial.begin(9600);
  #endif

	// Inicializar el LCD
	lcd.init();

	//Encender la luz de fondo.
	lcd.backlight();

	// Escribimos el Mensaje en el LCD de bienvenida
	lcd.clear();
	lcd.print(F(" Sistema Alarma "));
	lcd.setCursor(0, 1); // [col,row]
	lcd.print(F("  UltraFreezer  "));
  delay(1000);

  // inicializo sensor temperatura interno
  // Begin serial communication with Arduino and SIM800
  mySim.begin(9600);
  mySim.setLcd(&lcd);
  
  // inicializo el pin PWM para la carga de la bateria
  pinMode(PWM_BAT, OUTPUT);
  
  // inicializo referencia del ADC
  ADCvRef = vRefADC(ADCREF50);
  #ifdef DEBUGBAT
    Serial.print(F("ADCvRef:  "));
    Serial.print(ADCvRef, 3);
    Serial.println(F(" Volts"));
  #endif

  // inicializo pin que lee entrada 220VAC o bateria
  flagBat.begin(PWR_BAT);
  Bat.begin(ADCvRef);

  // inicializo sensores de temperatura
  temp.begin(ADCvRef);

  // inicio el contador de tiempo. millis() empieza siempre en cero
  // al comienzo del programa, hardware reset, o encendido del Arduino
  // no es necesario tener la variable start.
  actual = millis(); 
  timeTexCounter = actual;
  timeTinCounter = actual;

  // en este lugar se llega al encender el equipo o en la restitucion
  // a 220VAC luego de un extenso corte que teminó por descargar la
  // bateria. En consecuencia, al reiniciarse el sistema, se informa
  // por SMS.
  delay(5000);
  MedicionTemperaturas();
  strcpy(strOut, strSistemaOnline);
  strcat(strOut, strTemps);
  mySim.sendMessageSMS(strOut);

  // espero para arrancar el loop
  delay(2000);
}

void loop()
{
  //------------------------------------------------------------------
  // bloque de medicion de temperatura
  MedicionTemperaturas();

  //------------------------------------------------------------------
  // bloque de ajuste y cuenta de tiempo
  ProtocoloAlarmas();

  //------------------------------------------------------------------
  // bloque de carga de bateria (elije entre cargar o no)
  EstadoCargaBateria();

  // -----------------------------------------------------------------
  // espero cinco segundos para continuar
  while(millis()-actual < 5000);
}

// -----------------------------------------------------------------
// funcion que mide ambas temperaturas y las convierte a strings
// para facilitar su manejo
// -----------------------------------------------------------------
void MedicionTemperaturas(void)
{
  temp.MedirTemperaturaInterna();
  temp.TemperaturaTostring(strTempInt, temp.GetTempInterna(), " Freezer");  // Freezer -65*C <-- 13
  #ifdef DEBUGTEMP
    Serial.print("Tint:  ");
    Serial.print(temp.GetTempInterna(), 1);
    Serial.println("°C");
  #endif

  temp.MedirTemperaturaExterna();
  temp.TemperaturaTostring(strTempExt, temp.GetTempExterna(), " Room");     // Room 25*C <-- 9
  #ifdef DEBUGTEMP
    Serial.print("Text:  ");
    Serial.print(temp.GetTempExterna(), 1);
    Serial.println("°C");
  #endif

  // genero un unico string con ambas temperaturas, para despues
  // agregar a los distintos mensajes
  sprintf(strTemps, "\r\n%s\r\n%s", strTempInt, strTempExt);

  // mostrar temperatura en LCD
	lcd.clear();
 	lcd.print(strTempInt);
	lcd.setCursor(0, 1);
 	lcd.print(strTempExt);
}

// -----------------------------------------------------------------
// funcion que maneja todo el protocolo de alarmas... cuando avisar
// y en que avisar
// -----------------------------------------------------------------
void ProtocoloAlarmas(void)
{
  // obtengo el tiempo actual en milisegundos desde que se reseteo el
  // arduino, se encendio, etc.
  actual = millis();

  // -----------------------------------------------------------------
  // me fijo si cambio de 220 a bateria y viceversa, aviso si hubo un cambio de estado
  if(flagBat.isChanged() == true)
  {
    lcd.clear();
    if(flagBat.getState() == HIGH)
    {
      // el sistema paso de bateria a conectado a red
      // (volvio la luz)
      #ifdef DEBUGBAT
        Serial.println(strSistema220);
      #endif
      lcd.print(strSistema220);
      strcpy(strOut, strSistema220);
    }
    else
    {
      // el sistema paso de conectado a red a bateria
      // (se corto la luz)
      #ifdef DEBUGBAT
        Serial.println(strSistemaBat);
      #endif
      lcd.print(strSistemaBat);
      strcpy(strOut, strSistemaBat);
    }
    // agrego las temperaturas y aviso el estado
    // del sistema (220 o bateria + temperaturas)
    strcat(strOut, strTemps);
    mySim.sendMessageSMS(strOut);
  }

  // -----------------------------------------------------------------
  // pregunto por la temparatura interna y envio alarma
  if(temp.GetTempInterna() >= TEMPINTALARM)
  {
    switch(countTinAlarm)
    {
      case 0:
        // para evitar falsas alarmas, primero espero 5 minutos
        // si la temperatura sigue siendo mayor a la de alarma, 
        // envio la primera alarma y paso el contador de alarmas a 1
        if((actual-timeTinCounter) >= 5*UNMINUTO)
        {
          lcd.clear();
          lcd.print(lcdAlarmaFreezer);
          strcpy(strOut, strAlarmaTemperatura);
          strcat(strOut, strTempInt);
          mySim.sendMessageSMS(strOut);
          timeTinCounter = actual; // reinicio la cuenta para una hora
          countTinAlarm = 1; // reinicio la cuenta la cantidad de avisos
        }
        break;
      case 1:
        // en este caso ya hubo una alarma a los 5 minutos de que la temperatura
        // pasó el limite. En este caso, luego de una hora envio una segunda
        // alarma e incremento el contador. 
        if((actual-timeTinCounter) >= UNAHORA)
        {
          lcd.clear();
          lcd.print(lcdAlarmaFreezer);
          strcpy(strOut, strAlarmaTemperatura);
          strcat(strOut, strTempInt);
          mySim.sendMessageSMS(strOut);
          timeTinCounter = actual; // reinicio la cuenta para una hora
          countTinAlarm = 2; // reinicio la cuenta la cantidad de avisos
        }
        break;
      default: 
        // si llegue hasta aqui es porque ya no envio mas alarmas y se debera esperar
        // a que la temperatura descienda por debajo de la temperatura critica
        break;
    }
  }
  else
  {
    if(countTinAlarm > 0)
    {
      // si estoy aca es porque la temperatura descendio a valores normales
      // y aviso que ya esta todo bien
      strcpy(strOut, strSistemaOnline);
      strcat(strOut, strTemps);
      mySim.sendMessageSMS(strOut);
      // reseteo el contador de tiempo
      resetMillis();
    }
    // estoy en el caso normal, cuando todo funciona bien y las temperaturas
    // son las correctas. Aseguro el contador de avisos a cero
    countTinAlarm = 0;
    timeTinCounter = actual;
  }
  
  // -----------------------------------------------------------------
  // pregunto por la temparatura externa y envio alarma
  if(temp.GetTempExterna() >= TEMPEXTALARM)
  {
    switch(countTexAlarm)
    {
      case 0:
        if((actual-timeTexCounter) >= 5*UNMINUTO)
        {
          lcd.clear();
          lcd.print((lcdAlarmaAmbiente));
          strcpy(strOut, strAlarmaTemperatura);
          strcat(strOut, strTempExt);
          mySim.sendMessageSMS(strOut);
          timeTexCounter = actual; // reinicio la cuenta para una hora
          countTexAlarm = 1; // reinicio la cuenta la cantidad de avisos
        }
        break;
      case 1:
        if((actual-timeTexCounter) >= UNAHORA)
        {
          lcd.clear();
          lcd.print(lcdAlarmaAmbiente);
          strcpy(strOut, strAlarmaTemperatura);
          strcat(strOut, strTempExt);
          mySim.sendMessageSMS(strOut);
          timeTexCounter = actual; // reinicio la cuenta para una hora
          countTexAlarm = 2; // reinicio la cuenta la cantidad de avisos
        }
        break;
      default: 
        break;
    }
  }
  else
  {
    if(countTexAlarm > 0)
    {
      strcpy(strOut, strSistemaOnline);
      strcat(strOut, strTemps);
      mySim.sendMessageSMS(strOut);
      resetMillis();
    }
    timeTexCounter = actual;
    countTexAlarm = 0;
  }
  
  // -----------------------------------------------------------------
  // aviso que todo esta ok, una vez al mes
  if(actual >= UNMES)
  {
    mySim.sendMessageSMS(strSistemaOnline);
    mySim.sendMessageSMS(strTemps);
    resetMillis();
  }
}

// -----------------------------------------------------------------
// funcion que mide el estado de carga de bateria y decide si carga
// no en funcion de si esta enchufado el cargador
// -----------------------------------------------------------------
void EstadoCargaBateria(void)
{
  // me fijo si esta a 220 o a bateria y decido que hago con la carga
  if(flagBat.getState() == HIGH)
  {
    // conectado a 220VAC
    Bat.procesarCargaBateria();
    // indico abajo a la derecha del lcd que estoy conectado a 220VAC
  	lcd.setCursor(13, 1);
  	lcd.print("(L)");
    #ifdef DEBUGBAT
      Serial.print(F("Etapa: "));
      Serial.print(Bat.getEtapa());
      Serial.print(F(" - I_BAT: "));
      Serial.print(Bat.getCorriente(), 3);
      Serial.print(F(" - V_BAT: "));
      Serial.print(Bat.getTension(), 3);
      Serial.print(F(" - PWM: "));
      Serial.println(Bat.getPWM());
    #endif
  }
  else
  {
    // conectado a bateria
    Bat.resetCargaBateria();
    // indico abajo a la derecha del lcd que estoy conectado a bateria
	  lcd.setCursor(13, 1);
 	  lcd.print("(B)");
  }
}

// -----------------------------------------------------------------
// Reseteo el contador de tiempo del arduino, que se obtiene con "millis()"
// "c:\Users\IFIBIO\AppData\Local\Arduino15\packages\arduino\hardware\avr\1.8.6\cores\arduino\"
// la variable "timer0_millis" almacena el contador del TIMER0 que la incrementa cada milisengundo
// -----------------------------------------------------------------
// para evitar que el contador de tiempos de "millis()" desborde (lo que ocurre casi cada 50 dias),
// lo reseteo al mes cosa de hacer coincidir los tiempo. Tambien hay que resetar todas las
// variables que tengan que ver con la cuenta de tiempos
extern volatile unsigned long timer0_millis; 
void resetMillis(void)
{
  uint8_t oldSREG = SREG;
  cli();
  timer0_millis = 0;
  SREG = oldSREG;
  // reinicio la cuenta de tiempos
  timeTexCounter = 0;
  timeTinCounter = 0;
}

//-----------------------------------------------------------
// Funcion de referencia de tension del conversor ADC
//-----------------------------------------------------------
float vRefADC(unsigned char ref)
{
  // MUX3,2,1 en "1" indican que leo respecto a la referencia del ADC
  //ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  ADMUX = ref | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  getRefADC();
  delay(100);
  return getRefADC();
}

float getRefADC(void)
{
  long result;

  ADCSRA |= _BV(ADSC); // inicio una coonversion
  while(bit_is_set(ADCSRA, ADSC)); // terminó la conversion
  // leo el resultado de la conversion 
  result = ADCL; 
  result |= ADCH << 8; // de los 10bits del resultado
  // Back-calculate AVcc in Volts respect to 1.1 internal; 1125300 = 1.1*1023*1000
  result = 1125300L / result; 
  return (float) result / 1000.0; // en [Volts]
}
