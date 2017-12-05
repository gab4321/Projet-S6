/**
 * \file template.c
 *
 * \brief Empty application template
 *
 * Copyright (C) 2012-2014, Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 * Modification and other use of this code is subject to Atmel's Limited
 * License Agreement (license.txt).
 *
 * $Id: template.c 9267 2014-03-18 21:46:19Z ataradov $
 *
 */

/*- Includes ---------------------------------------------------------------*/

#include "astudio/template.h"


EtatAlerte cunt;

////////////////////////////////////////////////////////////////////////////////
// INTERRUPT SERVICE ROUTINE
////////////////////////////////////////////////////////////////////////////////

// ISR DU TIMER 1
ISR(TIMER1_COMPA_vect) {
	
	//compteur_color++; // test pour fonction de leds...
	
	
	if(compteurLEDAlerte++ >= intervalLEDAlerte)
	{
		compteurLEDAlerte = 0;
		//flagToggleLED = true;
		LED_Toggle();
	}
	
	
	// tests leds RGB
	//set_couleur(valeur_rouge, valeur_bleu, valeur_vert);
}

// ISR du timer 3: survient a chaque seconde -> pour timer les evenements de transmission & detection des sondes
ISR(TIMER3_COMPA_vect) {
	
	// compteur pour interval de mesure du pH
	if(compteurMesurePh++ >= intervalMesurePh)
	{
		compteurMesurePh = 0;
		flagPh = true;
	}
	
	/*
	// compteur pour lupdate / poll a la deuxieme sonde
	if(compteurPoll++ > intervalPoll)
	{
		compteurPoll = 0;
		flagPoll = true;
	}
	*/
	
	if(compteurAttenteAck1++ > TimeOutComm)
	{
		compteurAttenteAck1 = 0;
		flagISRTimeOutComm1 = true;		
	}
	
	
	// compteur de timeout de la comm si ne recoit pas dacknowledge UN (1) dans les delai requis
	if(compteurAttenteAck1++ > TimeOutComm && isWaitingAck1)//&& !perteConnexion)
	{
		compteurAttenteAck1 = 0;
		isWaitingAck1 = false; 
		flagTimeOutAck1 = true;
	}
	else if(!isWaitingAck1)
	{
		compteurAttenteAck1 = 0;
		flagTimeOutAck1 = false;
	}
	
	// compteur de timeout de la comm si ne recoit pas dacknowledge DEUX (2) dans les delai requis
	if(compteurAttenteAck2++ > TimeOutComm && isWaitingAck2)// && !perteConnexion)
	{
		compteurAttenteAck2 = 0;
		isWaitingAck2 = false; 
		flagTimeOutAck2 = true;
	}
	else if(!flagAttenteAck2)
	{
		compteurAttenteAck2 = 0;
		flagTimeOutAck2 = false;
	}
	
	// compteur qui part lalerte haute dans la condition ou la AlerteSonde1 = HAUT et AlerteSonde2 = BAS (si ca fait trop longtemps que la SONDE 1 attend)
	if(flagConfSonde1)
	{
		if(compteurTimeOutEmission1++ > TimeOutEmission1)
		{
			compteurTimeOutEmission1 = 0;
			flagTimeOutEmission1 = true;
		}
	}
	else
	{
		compteurTimeOutEmission1 = 0;
	}
	
	// compteur qui part lalerte haute dans la condition ou la AlerteSonde1 = BAS et AlerteSonde2 = HAUT (si ca fait trop longtemps que la SONDE 2 attend)
	if(flagConfSonde2)
	{
		if(compteurTimeOutEmission2++ > TimeOutEmission2)
		{
			compteurTimeOutEmission2 = 0;
			flagTimeOutEmission2 = true;
		}
	}
	else
	{
		compteurTimeOutEmission2 = 0;
	}
	
}

// ISR DE LADC
ISR(ADC_vect)
{
	ADCSRA &= 0b11101111; // disable le flag de lADC (vraiment utile ?)
	flag_ADC = true; // flag pour enabler la lecture dans le main
}




////////////////////////////////////////////////////////////////////////////////
// APPLICATION PRINCIPALE
////////////////////////////////////////////////////////////////////////////////
static void APP_TaskHandler(void)
{
	// on toggle la LED
	if(flagToggleLED)
	{
		flagToggleLED = false;
		LED_Toggle();
	}
	
	// Update les leds de niveau dalerte
	ledAlerte(etatAlerteGlobal);
	
	// Update les leds de niveau de pH
	ledPH(valeurPh);
	
	// Part la convertion dADC a chaque 30 secondes pour updater letat de la sonde
	if(flagPh)
	{
		flagPh = false;
		
		start_ADC();
		
		// MESSAGES DE DEBUG
		//////////////////////////////////////////////////////////
		//////////////////////////////////////////////////////////
		Ecris_UART("\n\rNiveau alerte global:%d\n\r", etatAlerteGlobal); // debug
	}

	// mesure ADC terminée: on calcul la valeur du pH et on update letat de la presente sonde
	if(flag_ADC)
	{
		flag_ADC = false;
		
		valeurADC = lecture_ADC();
		
		valeurPh = conv_PH(valeurADC);
		
		Ecris_UART("\n\rTension ADC: %f V\n\r",valeurADC);  // debug
		Ecris_UART("\n\rValeur pH: %f \n\r",valeurPh);		// debug
		
		// verifie le niveau de PH
		if(valeurPh == PH_ERROR_CODE)
		{
			Ecris_UART("\n\rATTENTION: Valeur du pH non valide\n\r"); // debug
		}
		else if(valeurPh < seuilPh)
		{
			//Ecris_UART("\n\rdetection pH trop faible: Alerte HAUT\n\r"); // debug
			
			if(niveauAlerte1 == BAS) // si changement detat on envoie un poll a lautre sonde
			{
				flagPoll = true;
				Ecris_UART("\n\rLe niveau dalerte sonde 1 passe de BAS a HAUT\n\r"); // debug
			}
			
			niveauAlerte1 = HAUT;
		}
		else if(valeurPh >= seuilPh)
		{
			//Ecris_UART("\n\rdetection pH OK: Alerte BAS\n\r"); // debug
			
			if(niveauAlerte1 == HAUT) // si changement detat on envoie un poll a lautre sonde
			{
				flagPoll = true;
				Ecris_UART("\nLe niveau dalerte sonde 1 passe de HAUT a BAS\n\r"); // debug
			}
			
			niveauAlerte1 = BAS;
		}
		
		Ecris_UART("\n\rLflagpoll %d\n\r", flagPoll); // debug
	}
	
	// Si on ne connait pas letat de la sonde 2 il faut lui demander
	if((niveauAlerte2 == INDETERMINE || niveauAlerte2 == ERROR_ALERTE) && (isWaitingAck1 == 0) && (isWaitingAck2 == false) && (perteConnexion == false))
	{
		envoieMessage(niveauAlerte1, POLL); // effectue le checksum + code dentete + envoie du poll
		
		
		
		//Ecris_UART("\NE CONNAIT PAS LETAT DE LA SONDE 2: ON LUI DEMANDE\n\r"); // debug
		Ecris_UART("\n\riswaitingAck1 = %d\n\r", isWaitingAck1); // debug
		Ecris_UART("\n\riswaitingAck2 = %d\n\r", isWaitingAck2); // debug
		
		isWaitingAck1 = 1; // on attend un ack1 de la sonde 2
		
		flagPoll = false;
	}
	
	// Si letat de la sonde 1 change il faut avertir lautre sonde
	if(flagPoll && (isWaitingAck1 == false) && (isWaitingAck2 == false) && (perteConnexion == false))
	{
		flagPoll = false;
		
		envoieMessage(niveauAlerte1, POLL); // effectue le checksum + code dentete + envoie du poll
		
		//Ecris_UART("\n\rCHANGEMENT DETAT SONDE 1: ON ENVOIE UN POLL\n\r"); // debug
		
		isWaitingAck1 = true; // on attend un ack1 de la sonde 2
	}
	
	// Si la sonde 2 na pas repondu dans linterval de timeout pour le ack1
	if(flagTimeOutAck1)
	{
		flagTimeOutAck1 = false;
		
		isWaitingAck1 = false; // sil y a un time out on arrete dattendre le ack 1
		
		niveauAlerte2 = INDETERMINE; // on ne sais pas le niveau dalerte de la sonde 2
		
		Ecris_UART("\n\rLa sonde 1 na pas recu de ack de sonde 2 dans le delai prescrit\n\r"); // debug
		
		if(compteurPerteConnexion++ >= maxPerteConnexion) // si ca trop dessai et pas de nouvelle de la sonde 2
		{
			perteConnexion = true;
			compteurPerteConnexion = 0;
			Ecris_UART("\n\rTrop dessais de comm: on rejette letat de la sonde 2 pour linstant\n\r"); // debug
		}
	}
	
	// Si la sonde 2 na pas repondu dans linterval de timeout pour le ack2
	if(flagTimeOutAck2)
	{
		flagTimeOutAck2 = false;
		
		isWaitingAck2 = false; // sil y a un time out on arrete dattendre le ack 2
		
		// on connait le niveau dalerte de la sonde 2 mais elle ne connait peut etre pas celui de la presente sonde
		/////////////////////////////////////////////////////////////////////////////////////////////////////
		if(!isWaitingAck1 && !flagPoll)		// si on nest pas deja en train dattendre la reponse a un poll
		flagPoll = true;	// la sonde 2 na peut etre pas recu le ack1 correctement donc la presente sonde va
		// initier elle meme un poll pour transmettre son état
		/////////////////////////////////////////////////////////////////////////////////////////////////////
	}
	
	// si un paquet est recu sur le wireless
	if(receivedWireless == 1)
	{
		Ecris_UART("\n\rReception dun message wireless\n\r"); // debug
		
		if(recoieMessage(&tempNiveauAlerte2, &receptAckType, &CRC_confirm))
		{
			if (!CRC_confirm)
			{
				envoieMessage(niveauAlerte1, ERROR_ACK); // envoie un message vide avec ack = error
				
				receivedWireless = 0; // la reception est terminée
			}
			if (CRC_confirm && receptAckType == ERROR_ACK && isWaitingAck1 == true)
			{
				envoieMessage(niveauAlerte1, POLL); // envoie le niveau dalerte son de 1 avec ack = poll
				
				receivedWireless = 0; // la reception est terminée
			}
			if (CRC_confirm && receptAckType == ERROR_ACK && isWaitingAck2 == true)
			{
				envoieMessage(niveauAlerte1, ACK1); // envoie le niveau dalerte de la sonde 1 avec ack = ack1
				
				receivedWireless = 0; // la reception est terminée
			}
			if (CRC_confirm && receptAckType == ACK1 && isWaitingAck1 == true)
			{
				isWaitingAck1 = false;
				
				niveauAlerte2 = tempNiveauAlerte2; // on update letat dalerte de la sonde 2
				
				if(niveauAlerte2 == HAUT)
				Ecris_UART("\n\rACK1:Reception niveau alerte sonde 2: HAUT\n\r"); // debug
				else if(niveauAlerte2 == BAS)
				Ecris_UART("\n\rACK1:Reception niveau alerte sonde 2: BAS\n\r"); // debug
				else
				Ecris_UART("\n\rACK1:Reception niveau alerte sonde 2: INDETERMINE\n\r"); // debug
				
				envoieMessage(niveauAlerte1, ACK2); // envoie un message vide avec ack = ack2
				
				receivedWireless = 0; // la reception est terminée
			}
			if (CRC_confirm && receptAckType == ACK2 && isWaitingAck2 == true)
			{
				isWaitingAck2 = false;
				
				Ecris_UART("\n\rACK2 recieved end transmission\n\r"); // debug
				
				receivedWireless = 0; // la reception est terminée
			}
			if (CRC_confirm && receptAckType == POLL)
			{
				envoieMessage(niveauAlerte1, ACK1); // envoie le niveau dalerte de la sonde 1 avec ack = ack1
				
				niveauAlerte2 = tempNiveauAlerte2; // on update letat dalerte de la sonde 2
				
				if(niveauAlerte2 == HAUT)
				Ecris_UART("\n\rPOLL:Reception niveau alerte sonde 2: HAUT\n\r"); // debug
				else if(niveauAlerte2 == BAS)
				Ecris_UART("\n\rPOLL:Reception niveau alerte sonde 2: BAS\n\r"); // debug
				else
				Ecris_UART("\n\rPOLL:Reception niveau alerte sonde 2: INDETERMINE\n\r"); // debug
				
				isWaitingAck2 = true; // on attend de recevoir la confirmation de lautre sonde
				
				receivedWireless = 0; // la reception est terminée
				
				perteConnexion = false; // on recoit un poll -> la sonde 2 vient de se reconnecter (en cas de perte de connexion)
				
				compteurPerteConnexion = 0; // on recoit un poll -> la sonde 2 vient de se reconnecter (en cas de perte de connexion)
			}
		}
		if(!(recoieMessage(&tempNiveauAlerte2, &receptAckType, &CRC_confirm)))
		{
			// LE MESSAGE NEST PAS DESTINÉ A LA SONDE / ON NE FAIT RIEN AVEC
			receivedWireless = 0; // la reception est terminée
		}
	}
	
	// On compute ici avec quelques conditions si la presente sonde doit emettre des ultrasons ou non
	////////////////////////////////////////////////////////////////////////////////
	
	// la sonde 2 nest plus connecté on prend en compte seulement la sonde 1
	if(perteConnexion && niveauAlerte1 == HAUT)
	etatAlerteGlobal = EMISSION;
	
	// la sonde 2 nest plus connecté on prend en compte seulement la sonde 1
	if(perteConnexion && niveauAlerte1 == BAS)
	etatAlerteGlobal = ATTENTE;
	
	// la sonde 1 a une alerte haute mais on ne connait pas encore letat de la sonde 2
	if(!perteConnexion && niveauAlerte1 == HAUT && (niveauAlerte2 == INDETERMINE || niveauAlerte2 == ERROR_ALERTE))
	etatAlerteGlobal = AVERTISSEMENT;
	
	// la sonde 1 a une alerte basse mais on ne connait pas encore letat de la sonde 2
	if(!perteConnexion && niveauAlerte1 == BAS && (niveauAlerte2 == INDETERMINE || niveauAlerte2 == ERROR_ALERTE))
	etatAlerteGlobal = ATTENTE;

	// la sonde 1 et la sonde 2 ont un etat dalerte bas
	if(!perteConnexion && niveauAlerte1 == BAS && niveauAlerte2 == BAS)
	etatAlerteGlobal = ATTENTE;
	
	// la sonde 1 et la sonde 2 ont un etat dalerte haut
	if(!perteConnexion && niveauAlerte1 == HAUT && niveauAlerte2 == HAUT)
	etatAlerteGlobal = EMISSION;
	
	// la sonde 1 alerte bas et sonde 2 alerte haut
	if(!perteConnexion && niveauAlerte1 == BAS && niveauAlerte2 == HAUT)
	etatAlerteGlobal = AVERTISSEMENT;

	// la sonde 1 a une alerte haute et la sonde 2 reste a alerte basse
	if(!perteConnexion && niveauAlerte1 == HAUT && niveauAlerte2 == BAS)
	{
		flagConfSonde1 = true;
		
		if(flagTimeOutEmission1)
		etatAlerteGlobal = EMISSION;
		else
		etatAlerteGlobal = AVERTISSEMENT;
	}
	else
	{
		flagConfSonde1 = false;
		isCountingAttenteSonde1 = false;
		flagTimeOutEmission1 = false;
	}
	////////////////////////////////////////////////////////////////////////////////
	
}




////////////////////////////////////////////////////////////////////////////////
// MAIN
////////////////////////////////////////////////////////////////////////////////
int main(void)
{
  SYS_Init();
   
  while (1)
  {
    PHY_TaskHandler(); //stack wireless: va vérifier s'il y a un paquet recu
    APP_TaskHandler(); //l'application principale roule ici
  }
}



////////////////////////////////////////////////////////////////////////////////
// FONCTIONS
//////////////////////////////////////////////////////////////////////////////// 
void LED_Toggle(void) {
	PORTD ^= 0x01; // change letat de la led branché sur la pin PD0
}

// gere les leds du niveau dalerte
void ledAlerte(EtatAlerteGlobal etatalerteglobal)
{
	///////////////////////////////////////////////////////////////////////////////////////
	// A TITRE DEXEMPLE: POSSIBILITÉ DE CODER DES LUMIERE QUI FLACHENT OU CODE DE COULEUR
	///////////////////////////////////////////////////////////////////////////////////////
	
	// sur port D
	
	if(etatalerteglobal == ATTENTE) // allume leds vert et eteint les autres
	{
		intervalLEDAlerte = 30000; // toggle a chaque 1 sec
	}
	else if(etatalerteglobal == AVERTISSEMENT) // allume leds bleu et eteint les autres
	{
		intervalLEDAlerte = 7000; // toggle a chaque 0.5 sec	
	}
	else if(etatalerteglobal == EMISSION) // allume leds rouge et eteint les autres
	{
		intervalLEDAlerte = 2500; // toggle a chaque 0.2 sec
	}
	else
	{
	
	}
}

// gere les leds du niveau de pH
void ledPH(float valeurPh)
{
	///////////////////////////////////////////////////////////////////////////////////////
	// A TITRE DEXEMPLE: POSSIBILITÉ DE CODER DES LUMIERE QUI FLACHENT OU CODE DE COULEUR
	///////////////////////////////////////////////////////////////////////////////////////
	
	if((valeurPh <= 7.0) && (valeurPh >= 4.0)) // allume leds vert et eteint les autres
	{
		PORTB |= 0x40; // (ROUGE)
		PORTB &= 0xFD; // (BLEU)
		PORTB |= 0x20; // (VERT)
		
		//PORTB &= 0xBF; // (ROUGE)
		//PORTB &= 0xFD; // (BLEU)
		//PORTB |= 0x20; // (VERT)
	}
	else if(valeurPh > 7.0) // allume leds bleu et eteint les autres
	{
		PORTB |= 0x40; // (ROUGE)
		PORTB |= 0x02; // (BLEU)
		PORTB &= 0xDF; // (VERT)
		
		//PORTB &= 0xBF; // (ROUGE) 
		//PORTB |= 0x02; // (BLEU) 
		//PORTB &= 0xDF; // (VERT)  
	}
	else if(valeurPh < 4.0) // allume leds rouge et eteint les autres
	{
		//PORTB |= 0x40; // (ROUGE) 
		//PORTB &= 0xFD; // (BLEU)  
		//PORTB &= 0xDF; // (VERT) 
		
		PORTB &= 0xBF; // (ROUGE)
		PORTB |= 0x02; // (BLEU)
		PORTB |= 0x20; // (VERT)
	}
	else
	{
		//PORTB &= 0xBF; // (ROUGE) 
		//PORTB &= 0xFD; // (BLEU)  
		//PORTB &= 0xDF; // (VERT)  
		
		PORTB |= 0x40; // (ROUGE)
		PORTB |= 0x02; // (BLEU)
		PORTB |= 0x20; // (VERT)
	}
}

//FONCTION D'INITIALISATION
void SYS_Init(void)
{
	cli();
		
	receivedWireless = 0;
	wdt_disable(); 
	init_UART();
	//PHY_Init(); //initialise le wireless
	//PHY_SetRxState(1); //TRX_CMD_RX_ON
	
	//wdt_disable();
	LED_setup();
	ADC_setup();
	Timer_Init();
	
	sei();	
}

// FONCTION DE CONVERSION DU VOLTAGE EN PH
float conv_PH(float adc_value)
{
	float PH = 0;
	
	PH = adc_value*(6.8+3.3)/6.8*3.5; // 6.8 et 3.3 sont pour le diviseur resistif
	
	if(PH > 14 || PH < 0)
		PH = PH_ERROR_CODE;
	
	return PH;
}

// set les couleurs RGB de niveaux 0 a 100
void set_couleur(int rouge, int bleu, int vert)
{
	if(compteur_color >= 100)
	compteur_color = 0; // reset compteur pour les leds
	
	// pour le rouge
	if((compteur_color >= (100 - rouge)))
	PORTB &= 0xBF; // niveau low = leds alumée
	else if(compteur_color < (100 - rouge))
	PORTB |= 0x40; // niveau high = leds éteinte
	
	// pour le bleu
	if((compteur_color >= (100 - bleu)))
	PORTB &= 0xFD; // niveau low = leds alumée
	else if(compteur_color < (100 - bleu))
	PORTB |= 0x02; // niveau high = leds éteinte

	// pour le vert
	if((compteur_color >= (100 - vert)))
	PORTB &= 0xDF; // niveau low = leds alumée
	else if(compteur_color < (100 - vert))
	PORTB |= 0x20; // niveau high = leds éteinte
	
	///////////////////////////////////////////
	// MODIF POUR POUVOIR FAIRE LES DEUX SERIES DE LEDS
	///////////////////////////////////////////
}

// INIT TIMER
void Timer_Init(void)
{
	//Disable timer interrupts
	TIMSK1 = 0x00;
	TIMSK3 = 0x00;
	
	// timer 1
	TCCR1A = 0x00; //Timer not connected to any pins
	TCCR1B = 0x09; //CTC mode; Timer_Rate = System_CLK = 1MHz
	// 1 tick = 1 us
	TCNT1 = 0x0000; //Clear timer
	OCR1A = 100; //Load compare value with desired delay in us
	
	// timer 3
	TCCR3A = 0x00;
	TCCR3B = 0x0D;
	
	TCNT3 = 0x0000; // Clear timer
	OCR3A = 7813; // environ 1 seconde entre chaque interrupt
	
	//Enable OCIE1A & OCIE3A Interrupt
	TIMSK1 = 0x02;
	TIMSK3 = 0x02;
}

// INIT ADC
void ADC_setup(void)
{
	DDRF = 0x00; // port F input pour lADC
	
	ADMUX = 0b01000000;
	
	ADCSRA = 0b10001111;
}

// INIT PORT LEDs
void LED_setup(void)
{
	///////////////////////////////////////////
	// POUR PORT LED PH
	///////////////////////////////////////////
	
	// init ports des LEDs (port B)
	DDRB = 0x00; // initialisation
	
	DDRD = 0x00; // initialisation
		
	DDRB |= 0x40; //PB6 output (rouge) (pour ph)
		
	DDRB |= 0x20; //PB5 output (bleu) (pour ph)
		
	DDRB |= 0x02; //PB1 output (vert) (pour ph)
	
	DDRD |= 0x01; //PD0 output (rouge) (pour niveau alerte)
		
	//PORTB = 0xFF; //LEDs a off
	PORTB = 0x00;	//LEDs a off
	
	//PORTD = 0xFF; //LEDs a off
	PORTD = 0x00;	//LEDs a off
	
	//
	
	///////////////////////////////////////////
	// POUR PORT LED PH
	///////////////////////////////////////////
	
	// A FAIRE **
}

// FONCTION QUI EFFECTUE LA LECTURE DE LADC
float lecture_ADC(void)
{
	float volt = 0;
	float Offset_ADC = 0.1;
	
	int ADC_value_low = 0;
	int ADC_value_high = 0;
	int ADC_value = 0;
	
	ADC_value_low = (unsigned int)ADCL; // registre low a lire en premier
	ADC_value_high = (unsigned int)ADCH; // registre high a lire en deuxieme
			
	ADC_value = ((ADC_value_high<<8)&0x0300)|ADC_value_low; // assemble les valeurs des registres
	
	volt = (float)(ADC_value)/1024.0*3.3 + Offset_ADC;
			
	return volt;
}

// Lis uart pour scanf
char Lis_UART(void)
{

char data = 0; 

	if(UCSR1A & (0x01 << RXC1))
	{
		data = UDR1;
	}
	
return data;
}

// ecrire comme printf (fonction a brunet)
void Ecris_UART(char *data, ...)
{
	char buffer[256];
	va_list args;
	va_start (args, data);
	vsprintf (buffer,data, args); //cree une chaine de caracteres en formattant les arguments en entree

	char i = 0;
	while( buffer[i] != (char)0 ) //tant qu'on n'est pas arrivé à la fin de la chaîne de caractères
	{
		UDR1 = buffer[i];	//transmet le caractere en cours
		while(!(UCSR1A & (0x01 << UDRE1)));	//attend que le caractère soit fini d'envoyer
		i++;	//incrémenter l'index du caractere en cours
	}

	va_end (args);
}

// init uart
void init_UART(void)
{
	//baud rate register = Fcpu / (16*baud rate - 1)
	//baudrate = 9600bps //s'assurer que la fuse CLKDIV8 n'est pas activée dans les Fuses, sinon ca donne 1200bps
	UBRR1H = 0x00;
	UBRR1L = 53;
	
	UCSR1A = 0x00;
	UCSR1B = 0x18; //receiver, transmitter enable, no parity
	UCSR1C = 0x06; //8-bits per character, 1 stop bit
}

void start_ADC()
{
	ADCSRA |= 0b01000000;
}


//Fonctions pour la communication
bool genMessage(EtatAlerte alerte, char *message, AckType ack)
{

	// formatage du Id mdp
	memcpy(message, IDCode, strlen(IDCode));

	// formatage du ack
	memcpy(message+strlen(IDCode), &ack, sizeof((uint8_t)ack));
	//message[strlen(IDCode)] = ack;
	
	
	// formatage du message
	memcpy(message+strlen(IDCode)+sizeof((uint8_t)ack), &alerte, sizeof((uint8_t)alerte));

	uint8_t crcResult = crc_8((unsigned char*)message, MessageLength-1);

	//memcpy(message+(sizeof(message)- sizeof(crcResult)), &crcResult, sizeof(crcResult));
	memcpy(message+(MessageLength-1), &crcResult, sizeof(crcResult));
	//memcpy(message+(126), &crcResult, 1);

	//uint8_t crcResult2 = crc_8((unsigned char*)message, MessageLength);

	Ecris_UART("Message sent");

	return true; //a changer
	
}

bool decodeMessage(EtatAlerte *alerte, char *message, AckType *ack, bool *crcConfirm)
{
	bool status = false;
	//testeu = memcmp(message, IDCode, strlen(IDCode));


	if(!memcmp(message, IDCode, strlen(IDCode)))
	{
		//memcpy(*ack, &message+strlen(IDCode),1);
		*ack = message[strlen(IDCode)];
		*alerte = message[strlen(IDCode)+1];



		//memcpy(*alerte, &message+strlen(IDCode+1), 1);
		
		if (crc_8((unsigned char*)message, MessageLength)==0)
		{
			*crcConfirm = true;
		}
		else
		{
			*crcConfirm = false;
		}
		
		
		
		status = true;
	}

	return status;
}


// analyse le message recu wireless et sort le tableau recu en eliminant lentete / gestion derreurs
bool recoieMessage(EtatAlerte *alerte, AckType *acktype, bool *CRC_confirm)
{
	bool status = false;

	uint8_t tempData[MessageLength];
	//sprintf(tempData, "%s" , ind.data);
	//memcpy(tempData, ind.data, MessageLength)
	//if(decodeMessage(alerte, tempData, acktype, CRC_confirm))
	if(decodeMessage(alerte, ind.data, acktype, CRC_confirm))
	{
		status = true;
		
	}
	return status;
}

// envoie le message par communication sans fils
void envoieMessage(EtatAlerte alerte, AckType acktype)
{
	
/*	// variables pour test
		uint8_t testBuffer[MessageLength];
		Alerte testAlerte = HAUT;
		uint8_t ackTest = 2;
		*/
	genMessage(alerte, messageBuffer, acktype);
	Ecris_Wireless(messageBuffer, MessageLength+10);

}
