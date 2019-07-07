 
#include "TimerOne.h"
#include <EEPROM.h>

// ENTRADAS
const int WasherPin       = 2;
const int SeatBeltPin     = 4;

//SALIDAS
const int IgnRelayPin     = 8;
const int WasherRelayPin  = 9;
const int WiperRelayPin   = 10;
const int AUX             = 11;
const int ABSRelayPin     = 13;

//CONSTANTES
const boolean activado    = false;
const boolean desActivado = true;

const byte OnLeftAddress  = 12;
const byte OnAdress1      = 2;
const byte OnTimes1        = 5;
const byte OnAdress2      = 5;
const byte OnTimes2       = 20;
const byte errorBlinkSpeed = 15;
const byte errorMinBright = 40;
const byte errorMaxBright = 250;

const int contIgnMaxTime  = 150;   //7.50 secs
const int ABSApagar       = 60;    //3 secs
const int delayLuz        = 200;
const int timeToOnWasher  = 15;
const int MaxTimeToPressButton = 16;
const int MaxTimeWaiting  = 100;
const int TimeBypassMin   = 200;
const int TimeBypassMax   = 220;

//PASOS
// para ignicion
const byte Inicio          = 0;
const byte cinturon        = 1;
const byte codigoPresionar = 2;
const byte codigoSoltar    = 3;
const byte espera          = 4;
const byte ignicion        = 5;
const byte WipWasher       = 6;
const byte WasherStep      = 7;

// para errores y codigos
const byte errorState      = 10;

// para bypasss
const byte bypassWait      = 20;
const byte bypassCode      = 21;
const byte bypassErase     = 22;
const byte bypassWrite1    = 23;
const byte bypassWrite2    = 24;
const byte bypassIgnition  = 25;

//VARIABLES
boolean WasherInput       = true;
boolean WasherLast        = true;
boolean WasherAscendente  = false;
boolean WasherDescendente = false;
boolean SeatBeltInput     = true;

byte wipSpeed             = 0;
byte paso                 = Inicio;
byte washerPressCount     = 0;
byte EEPROMOnLeftValue    = 0;

int contador              = 0;
int contadorLast          = -1;
int contAscendente        = 0;
int contDescendente       = 0;
int wiperCounter          = 0;
int bypassCounter         = 0;

 
void setup()
{
  //PINES
  pinMode(IgnRelayPin, OUTPUT);
  pinMode(WiperRelayPin, OUTPUT);
  pinMode(WasherRelayPin, OUTPUT);      
  pinMode(ABSRelayPin, OUTPUT);


  pinMode(WasherPin, INPUT_PULLUP);
  pinMode(SeatBeltPin, INPUT_PULLUP);

  //PRIMER ESTADO
  digitalWrite(IgnRelayPin, desActivado);
  digitalWrite(WiperRelayPin, desActivado);
  digitalWrite(WasherRelayPin, activado);
  digitalWrite(ABSRelayPin, activado);
  
  //TIMER ONE
  Timer1.initialize(50000);
  Timer1.attachInterrupt(callback);
}
 
// INTERRUPCION
void callback()
{
  WasherInput = digitalRead(WasherPin);
  if (WasherLast != WasherInput)
  {
    if (!WasherInput)
    {
      digitalWrite(WasherRelayPin, activado);
      WasherAscendente = true;
      contAscendente = contador;
    }
    else
    {
      WasherDescendente = true;
      contDescendente = contador;
    }
    contador = 0;
  }
  WasherLast = WasherInput;
  contador++;
}

// CICLO
void loop()
{
  if (contador != contadorLast)
  {

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (paso == Inicio){
      delay(50);
      if (!WasherInput){
        paso = bypassWait;
      }
      else{
        
        EEPROMOnLeftValue = EEPROM.read(OnLeftAddress);
        if (EEPROMOnLeftValue > 0)
        {
          digitalWrite(ABSRelayPin, desActivado);
          delay(500);
          for (byte val = EEPROMOnLeftValue; val > 0; val--) 
          {
            delay(delayLuz);
            digitalWrite(ABSRelayPin, activado);
            delay(delayLuz);
            digitalWrite(ABSRelayPin, desActivado);
          }
          EEPROMOnLeftValue = EEPROMOnLeftValue -1;
          EEPROM.update(OnLeftAddress, EEPROMOnLeftValue);
          paso = ignicion;
        }
        else
          paso = codigoPresionar;
//          paso = cinturon;
      }
    }
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if (paso == cinturon)
    {
      SeatBeltInput = digitalRead(SeatBeltPin);
      if (SeatBeltInput == desActivado){                      // sin cinturon
        if (contador > 40)
          paso = errorState;
        else
          paso = cinturon;
      }               
      else
        paso = codigoPresionar;                              // con cinturon
    }
    
    // Paso 2
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if (paso == codigoPresionar)
    {
      if (contador == ABSApagar)                      //APAGAR ABS al iniciar
        digitalWrite(ABSRelayPin, desActivado);
      if (WasherAscendente)                           // se halo washer
      {
        if (contAscendente < ABSApagar)               //ERROR poco tiempo
          paso = errorState;
        else if (contAscendente < ABSApagar + 20)     // 1 seg para persionar
          paso = codigoSoltar;
        else                                          //ERROR
          paso = errorState;
        WasherAscendente = false;
      }
      if (WasherDescendente)                          //ERROR
      {
        paso = errorState;
        WasherDescendente = false;
      }

      if (contador > MaxTimeWaiting)
        paso = errorState;
    }
  
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if (paso == codigoSoltar)
    {
      if (contador == 40)                                     //punto de liberacion
        digitalWrite(ABSRelayPin, activado);
      if (WasherDescendente)                                  //se libera
      {
        if (contDescendente > 40 && contDescendente < 50)     //se libero a tiempo
          paso = espera;
        else                                                  //ERROR
          paso = errorState;
        WasherDescendente = false;
      }
    }
    
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if (paso == espera)
    {
      if (WasherAscendente || WasherDescendente)              //ERROR, no se espero??
        paso = errorState;
      if (contador > 40)                                     //IGNICION
        paso = ignicion;
    }
    
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if (paso == ignicion)                                      //Reestablecer
    {
      digitalWrite(IgnRelayPin, activado);
      delay(100); 
      digitalWrite(IgnRelayPin, desActivado);
      digitalWrite(WasherRelayPin, desActivado);
      digitalWrite(ABSRelayPin, desActivado);
      paso = WipWasher;
    }


///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if (paso == WipWasher){
      if (WasherAscendente)
      {
        contador = 0;
        WasherAscendente = false;
      }
      else if (!WasherInput)                 //washer esta negado
      {
        if (contador > timeToOnWasher)
        {
          digitalWrite(WasherRelayPin, desActivado);
          digitalWrite(WiperRelayPin, activado);
        }
      }
      if (WasherDescendente)
      {
        digitalWrite(WasherRelayPin, activado);
        digitalWrite(WiperRelayPin, desActivado);
        WasherDescendente = false; 
        if (contDescendente < timeToOnWasher)
          washerPressCount = washerPressCount +1;
      }

      if ((contador > MaxTimeToPressButton) && WasherInput)
      {
        if ((wipSpeed != 0) && (washerPressCount == 1))
          wipSpeed = 0;
        else
          wipSpeed = washerPressCount;

        washerPressCount = 0;
        contador = 0;
        paso = WasherStep;
      }
    }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if (paso == WasherStep)
    {
      if (!WasherInput)
      {
        digitalWrite(WasherRelayPin, activado);
        paso = WipWasher;
      }
      else
      {
        if (wipSpeed == 1)
        {
          if (contador == 4)
            digitalWrite(WiperRelayPin, activado);
          else if (contador == 10 )
            digitalWrite(WiperRelayPin, desActivado);
          if (contador > 200)
            contador = 0;
        }
  
        else if (wipSpeed == 2)
        {
          if (contador == 4)
          {
            digitalWrite(WiperRelayPin, activado);
          }
          else if (contador == 10 )
            digitalWrite(WiperRelayPin, desActivado);
          if (contador > 100)
            contador = 0;
        }
        else if (wipSpeed == 3)
        {
          if (contador == 4)
            digitalWrite(WiperRelayPin, activado);
          else if (contador == 10 )
            digitalWrite(WiperRelayPin, desActivado);
          if (contador > 60)
            contador = 0;
        }
        else
          digitalWrite(WiperRelayPin, desActivado);
      }
    }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if (paso == bypassWait)
    {
      if (contador == TimeBypassMin)
        digitalWrite(ABSRelayPin, desActivado);
      if (WasherDescendente)
      {
        WasherDescendente = false; 
        if ((contDescendente > TimeBypassMin) && (contDescendente < TimeBypassMax))
          paso = bypassCode;
        else
          paso = bypassErase;
      }
   }

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
   else if (paso == bypassErase)
   {
      digitalWrite(ABSRelayPin, desActivado);
      delay(100);
      EEPROM.update(OnLeftAddress, 0);
      digitalWrite(ABSRelayPin, activado);
      delay(1000);
      digitalWrite(ABSRelayPin, desActivado);
      
      paso = errorState;
      
   }
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
   else if (paso == bypassCode)
   {
      if (WasherDescendente)
      {
        WasherDescendente = false; 
        if (contDescendente < MaxTimeToPressButton)
          bypassCounter = bypassCounter +1;
      }

      if ((contador > MaxTimeToPressButton) && WasherInput)
      {
        if (bypassCounter == OnAdress1)
          paso = bypassWrite1;
        else if (bypassCounter == OnAdress2)
          paso = bypassWrite2;
        else
          paso = errorState;

        bypassCounter = 0;
        contador = 0;
      }
   }
      
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
   else if (paso == bypassWrite1)
   {
      delay(500);
      digitalWrite(ABSRelayPin, desActivado);
      delay(100);
      digitalWrite(ABSRelayPin, activado);
      delay(100);
      digitalWrite(ABSRelayPin, desActivado);
      delay(100);
      digitalWrite(ABSRelayPin, activado);
      delay(100);
      digitalWrite(ABSRelayPin, desActivado);
      EEPROM.update(OnLeftAddress, OnTimes1);
      paso = bypassIgnition;
      
    }

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
   else if (paso == bypassWrite2)
   {
      delay(500);
      digitalWrite(ABSRelayPin, desActivado);
      delay(100);
      digitalWrite(ABSRelayPin, activado);
      delay(100);
      digitalWrite(ABSRelayPin, desActivado);
      delay(100);
      digitalWrite(ABSRelayPin, activado);
      delay(100);
      digitalWrite(ABSRelayPin, desActivado);
      EEPROM.update(OnLeftAddress, OnTimes2);
      paso = bypassIgnition;
      
    }
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if (paso == bypassIgnition){
      if (WasherInput)
        paso = ignicion;
    }

    // PASO 0             MODIFICAR
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
    else if (paso == errorState)
    {
        for (byte val1 = errorMinBright; val1 < errorMaxBright; val1++) {
          for (int val2 = 0; val2 < errorBlinkSpeed; val2++) {
            digitalWrite(ABSRelayPin, activado);
            delayMicroseconds(val1);
            digitalWrite(ABSRelayPin, desActivado);
            delayMicroseconds(255-val1);
          }
        }

        for (byte val1 = errorMaxBright; val1 > errorMinBright; val1--) {
          for (int val2 = 0; val2 < errorBlinkSpeed; val2++) {
            digitalWrite(ABSRelayPin, activado);
            delayMicroseconds(val1);
            digitalWrite(ABSRelayPin, desActivado);
            delayMicroseconds(255-val1);
          }
        }
    }

    contadorLast = contador;                //Actualiza Contador Last
  }
}
