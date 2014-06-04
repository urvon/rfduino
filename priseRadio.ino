/*
 * Code pour construction d'un recepteur "maison", recois un signal et ouvre ou ferme un port exterieur (reli� par exemple a un relais)
 * Fr�quence : 433.92 mhz
 * Protocole : home easy 
 * Licence : CC -by -sa
 * Mat�riel associ� : Atmega 328 (+r�sonateur associ�) + r�cepteur RF AM 433.92 mhz + relais + led d'etat
 * Auteur : Valentin CARRUESCO  aka idleman (idleman@idleman.fr - http://blog.idleman.fr)
 * 
 * Bas� sur le travail de :
 * Barnaby Gray 12/2008
 * Peter Mead   09/2009
 */
 
 #include <EEPROM.h>
 #include "EEPROMAnything.h"

int recepteurPin = 8;
int relaiPin = 10;
int ledPin = 11;

struct config_t
{
  long sender;
  int receptor;
} signal;

struct signal_t
{
  long sender;
  int receptor;
  boolean isSignal;
  boolean state;
} receivedSignal;


void setup()
{	pinMode(recepteurPin, INPUT);
        pinMode(relaiPin, OUTPUT);
        pinMode(ledPin, OUTPUT);
	Serial.begin(9600);
        Serial.println("start");
        digitalWrite(ledPin,HIGH);
}


void loop()
{
  
       //Ecoute des signaux
       listenSignal();
       
       //Si un signal au protocol home easy est re�u...
       if(receivedSignal.isSignal){
           Serial.println("######## GOOOOOOD  ###########");
           //R�cuperation du signal de r�f�rence en m�moire interne
           EEPROM_readAnything(0, signal);
         
           //Si aucun signal de r�f�rence, le premier signal re�u servira de reference en m�moire interne
           if(signal.sender <0){
              EEPROM_writeAnything(0, receivedSignal);
              Serial.println("Aucun signal de r�f�rence dans la m�moire interne, enregistrement du signal re�u comme signal de r�ference");
              EEPROM_readAnything(0, signal);
           }

           //Comparaison signal de r�f�rence, signal re�u
           if (receivedSignal.sender==signal.sender && receivedSignal.receptor==signal.receptor){
                     Serial.println("Signal correspondant");
                     
                    //On ferme ou on ouvre le relais en fonction du bit d'etat (on/off) du signal
                    if(receivedSignal.state)
        	        {	
                        Serial.println("Etat : on, fermeture du relais");
                        digitalWrite(relaiPin,HIGH);
        	        }
        	        else
        	        {
                        Serial.println("Etat : off, ouverture du relais");	
                        digitalWrite(relaiPin,LOW);
        	        }
                  //La led clignotte 10 fois rapidemment pour signaler que le signal est le bon
                  blinkLed(10,20);
           }else{
              Serial.println("Signal inconnu");	
              //La led clignotte 1 fois lentement  pour signaler que le signal est mauvais mais que le protocol est le bon
              blinkLed(1,200); 
           }
       }
     
}


void blinkLed(int repeat,int time){
 for(int i =0;i<repeat;i++){
          digitalWrite(ledPin,LOW);
          delay(time);
          digitalWrite(ledPin,HIGH);
          delay(time);
  } 
}


void listenSignal(){
        receivedSignal.sender = 0;
        receivedSignal.receptor = 0;
        receivedSignal.isSignal = false;
            
        int i = 0;
	unsigned long t = 0;

	byte prevBit = 0;
	byte bit = 0;

	unsigned long sender = 0;
	bool group = false;
	bool on = false;
	unsigned int recipient = 0;

	// latch 1
	while((t < 9200 || t > 11350))
	{	t = pulseIn(recepteurPin, HIGH, 1000000);  
	}
        Serial.println("latch 1");
      
	// latch 2
	while(t < 2550 || t > 2800)
	{	t = pulseIn(recepteurPin, HIGH, 1000000);
	}

        Serial.println("latch 2");

	// data
	while(i < 64)
	{
		t = pulseIn(recepteurPin, HIGH, 1000000);
                 
		if(t > 200 && t < 365)
		{	bit = 0;
                    
		}
		else if(t > 1000 && t < 1400)
		{	bit = 1;
                      
		}
		else
		{	i = 0;
			break;
                  Serial.println("bit mort"+t);
		}

		if(i % 2 == 1)
		{
			if((prevBit ^ bit) == 0)
			{	// must be either 01 or 10, cannot be 00 or 11
				i = 0;
				break;
			}

			if(i < 53)
			{	// first 26 data bits
				sender <<= 1;
				sender |= prevBit;
			}	
			else if(i == 53)
			{	// 26th data bit
				group = prevBit;
			}
			else if(i == 55)
			{	// 27th data bit
				on = prevBit;
			}
			else
			{	// last 4 data bits
				recipient <<= 1;
				recipient |= prevBit;
			}
		}

		prevBit = bit;
		++i;
	}

       
	// interpret message
	if(i > 0)
	{
            receivedSignal.sender = sender;
            receivedSignal.receptor = recipient;
            receivedSignal.isSignal = true;
            if(on)
    	    {
             receivedSignal.state = true;
            }else{
              receivedSignal.state = false;
            }
	}
}
