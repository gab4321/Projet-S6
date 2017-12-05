/*
 * code de test des fonctions du projet S6
 * Gabriel Guilmain
 */ 

////////////////////////////////////////////////////////////////////////////////
// INITIALISATION
////////////////////////////////////////////////////////////////////////////////

/*- Includes ---------------------------------------------------------------*/
#include "sys.h"
#include "phy.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>

/*- Definitions ------------------------------------------------------------*/
// Put your preprocessor definitions here
#define PH_ERROR_CODE 0xFFFF

//#define MESSAGE_ALERTE_HAUT "ACPDNERW" // a changer eventuellement !!!

/*- Types ------------------------------------------------------------------*/
// Put your type definitions here
typedef enum EtatAlerte EtatAlerte;
typedef enum EtatComm EtatComm;
typedef enum AckType AckType;
typedef enum EtatAlerteGlobal EtatAlerteGlobal;

/*- Prototypes -------------------------------------------------------------*/
// Put your function prototypes here
void start_ADC(void);
void SYS_Init(void);
void ADC_setup(void);
void LED_setup(void);
void Timer_Init(void);
void LED_Toggle(void);
//void printf_func(char* buf);

void ledAlerte(EtatAlerteGlobal etatalerte);
void ledPH(float valeurPh);

void set_couleur(int rouge, int bleu, int vert);
float lecture_ADC(void);
float conv_PH(float adc_value);

void envoieMessage(EtatAlerte message, AckType acktype);
bool recoieMessage(EtatAlerte message, AckType acktype, bool CRC_confirm);

char Lis_UART(void);
void Ecris_UART(char *data, ...);
void init_UART(void);
//void wdt_disable(void);

/*- Variables --------------------------------------------------------------*/
// Put your variables here
uint8_t receivedWireless;	//cette variable deviendra 1 lorsqu'un nouveau paquet aura été recu via wireless (et copié dans "PHY_DataInd_t ind"
							//il faut la mettre a 0 apres avoir géré le paquet; tout message recu via wireless pendant que cette variable est a 1 sera jeté

PHY_DataInd_t ind; //cet objet contiendra les informations concernant le dernier paquet qui vient de rentrer

enum EtatAlerte
{
	// niveaux dalerte
	BAS,
	HAUT,
	INDETERMINE,
	ERROR_ALERTE
};

enum EtatComm
{
	// etat de communication
	IDLE,
	ATTENTE_ACK1,
	ATTENTE_ACK2
};

enum AckType
{
	// type dacknowledge
	POLL,
	ACK1,
	ACK2,
	ERROR_ACK
};

enum EtatAlerteGlobal
{
	// niveaux dalerte global pour lemission ou non de pH
	ATTENTE,
	AVERTISSEMENT,
	EMISSION
};

volatile int compteurMesurePh = 0;
volatile int compteurAttenteConf = 0;
volatile int compteurAttenteEmission = 0;
volatile int compteurAttenteAck1 = 0;
volatile int compteurAttenteAck2 = 0;
volatile int compteur_color = 0;
volatile int compteurPoll = 0;
volatile int compteurTimeOutEmission1 = 0;
volatile int compteurTimeOutEmission2 = 0;
volatile int compteurLEDAlerte = 0;

volatile int intervalMesurePh = 2; // secondes
volatile uint8_t intervalAttenteAlerte = 30; // secondes
volatile uint8_t intervalAttenteEmission = 30; // secondes
volatile int intervalLEDAlerte = 5000;

volatile bool flagToggleLED = false;
volatile bool flagPh = false;
volatile bool flag_ADC = false;
volatile bool flagCancelAlerte = false;
volatile bool flagAttenteAlerte = false;
volatile bool flagAttenteEmission = false;
volatile bool flagEmissionFinie = false;
bool flagPoll = false;
volatile bool flagAttenteAck1 = false;
volatile bool flagAttenteAck2 = false;
volatile bool flagTimeOutAck1 = false;
volatile bool flagTimeOutAck2 = false;
volatile bool flagConfSonde1 = false;
volatile bool flagConfSonde2 = false;
volatile bool isCountingAttenteSonde1 = false;
volatile bool isCountingAttenteSonde2 = false;
volatile bool flagTimeOutEmission1 = false;
volatile bool flagTimeOutEmission2 = false;
volatile bool flagISRTimeOutComm1 = false;

volatile int intervalPoll = 20; // secondes
volatile int TimeOutComm = 5; // secondes
volatile int TimeOutEmission1 = 10; // secondes
volatile int TimeOutEmission2 = 10; // secondes

EtatAlerte niveauAlerte1 = INDETERMINE;
EtatAlerte tempNiveauAlerte2 = INDETERMINE;
EtatAlerte niveauAlerte2 = INDETERMINE;
EtatAlerteGlobal etatAlerteGlobal = ATTENTE;
AckType receptAckType = ERROR_ACK;

bool mesureStanby = false;
float valeurADC = 0;
float valeurPh = 7.0; // initialement PH neutre
uint8_t seuilPh = 4; // seuil du niveau 
uint8_t messageWireless[128];
uint8_t NbAlertesEnvoyee = 0;
uint8_t compteurPerteConnexion = 0;
uint8_t maxPerteConnexion = 5;
bool perteConnexion = false;
bool CRC_confirm = false;

volatile bool isWaitingAck1 = false;
volatile bool isWaitingAck2 = false;

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
		Ecris_UART("\nNiveau alerte global:%d\n", etatAlerteGlobal); // debug
	}

	// mesure ADC terminée: on calcul la valeur du pH et on update letat de la presente sonde
	if(flag_ADC)
	{
		flag_ADC = false;
		
		valeurADC = lecture_ADC();
		
		valeurPh = conv_PH(valeurADC); 
		
		Ecris_UART("\nTension ADC: %f V\n",valeurADC);  // debug
		Ecris_UART("\nValeur pH: %f \n",valeurPh);		// debug
		
		// verifie le niveau de PH
		if(valeurPh == PH_ERROR_CODE)
		{
			Ecris_UART("\nATTENTION: Valeur du pH non valide\n"); // debug
		}
		else if(valeurPh < seuilPh)
		{
			//Ecris_UART("\ndetection pH trop faible: Alerte HAUT\n"); // debug
			
			if(niveauAlerte1 == BAS) // si changement detat on envoie un poll a lautre sonde
			{
				flagPoll = true;
				Ecris_UART("\nLe niveau dalerte sonde 1 passe de BAS a HAUT\n"); // debug
			}
			
			niveauAlerte1 = HAUT; 
		}	
		else if(valeurPh > seuilPh)
		{
			//Ecris_UART("\ndetection pH OK: Alerte BAS\n"); // debug		
			
			if(niveauAlerte1 == HAUT) // si changement detat on envoie un poll a lautre sonde
			{
				flagPoll = true;
				Ecris_UART("\nLe niveau dalerte sonde 1 passe de HAUT a BAS\n"); // debug
			}
			
			niveauAlerte1 = BAS; 		
		}
		
		Ecris_UART("\nLflagpoll %d\n", flagPoll); // debug
	}
	
	// Si on ne connait pas letat de la sonde 2 il faut lui demander
	if((niveauAlerte2 == INDETERMINE || niveauAlerte2 == ERROR_ALERTE) && (isWaitingAck1 == 0) && (isWaitingAck2 == false) && (perteConnexion == false))
	{
		envoieMessage(niveauAlerte1, POLL); // effectue le checksum + code dentete + envoie du poll
		
		//Ecris_UART("\NE CONNAIT PAS LETAT DE LA SONDE 2: ON LUI DEMANDE\n"); // debug
		Ecris_UART("\iswaitingAck1 = %d\n", isWaitingAck1); // debug
		Ecris_UART("\iswaitingAck2 = %d\n", isWaitingAck2); // debug
			
		isWaitingAck1 = 1; // on attend un ack1 de la sonde 2
		
		flagPoll = false; 
	}	
	
	// Si letat de la sonde 1 change il faut avertir lautre sonde
	if(flagPoll && (isWaitingAck1 == false) && (isWaitingAck2 == false) && (perteConnexion == false))
	{
		flagPoll = false;
		
		envoieMessage(niveauAlerte1, POLL); // effectue le checksum + code dentete + envoie du poll
		
		//Ecris_UART("\nCHANGEMENT DETAT SONDE 1: ON ENVOIE UN POLL\n"); // debug
		
		isWaitingAck1 = true; // on attend un ack1 de la sonde 2
	}
	
	// Si la sonde 2 na pas repondu dans linterval de timeout pour le ack1 
	if(flagTimeOutAck1)
	{	
		flagTimeOutAck1 = false;
		
		isWaitingAck1 = false; // sil y a un time out on arrete dattendre le ack 1
		
		niveauAlerte2 = INDETERMINE; // on ne sais pas le niveau dalerte de la sonde 2
		
		Ecris_UART("\nLa sonde 1 na pas recu de ack de sonde 2 dans le delai prescrit\n"); // debug
		
		if(compteurPerteConnexion++ >= maxPerteConnexion) // si ca trop dessai et pas de nouvelle de la sonde 2
		{
			perteConnexion = true;
			compteurPerteConnexion = 0;
			Ecris_UART("\nTrop dessais de comm: on rejette letat de la sonde 2 pour linstant\n"); // debug		
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
		Ecris_UART("\nReception dun message wireless\n"); // debug
		
		if(recoieMessage(tempNiveauAlerte2, receptAckType, CRC_confirm))
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
					Ecris_UART("\nReception niveau alerte sonde 2: HAUT\n"); // debug
				else if(niveauAlerte2 == BAS)
					Ecris_UART("\nReception niveau alerte sonde 2: BAS\n"); // debug				
				else
					Ecris_UART("\nReception niveau alerte sonde 2: INDETERMINE\n"); // debug				
				
				envoieMessage(niveauAlerte1, ACK2); // envoie un message vide avec ack = ack2 
				
				receivedWireless = 0; // la reception est terminée
			}
			if (CRC_confirm && receptAckType == ACK2 && isWaitingAck2 == true)
			{
				isWaitingAck2 = false;
				
				receivedWireless = 0; // la reception est terminée
			}
			if (CRC_confirm && receptAckType == POLL)
			{
				envoieMessage(niveauAlerte1, ACK1); // envoie le niveau dalerte de la sonde 1 avec ack = ack1 
				
				niveauAlerte2 = tempNiveauAlerte2; // on update letat dalerte de la sonde 2
				
				if(niveauAlerte2 == HAUT)
					Ecris_UART("\nReception niveau alerte sonde 2: HAUT\n"); // debug
				else if(niveauAlerte2 == BAS)
					Ecris_UART("\nReception niveau alerte sonde 2: BAS\n"); // debug				
				else
					Ecris_UART("\nReception niveau alerte sonde 2: INDETERMINE\n"); // debug
									
				isWaitingAck2 = true; // on attend de recevoir la confirmation de lautre sonde
				
				receivedWireless = 0; // la reception est terminée
				
				perteConnexion = false; // on recoit un poll -> la sonde 2 vient de se reconnecter (en cas de perte de connexion)
				
				compteurPerteConnexion = 0; // on recoit un poll -> la sonde 2 vient de se reconnecter (en cas de perte de connexion)					
			}		
		}
		if(!(recoieMessage(tempNiveauAlerte2, receptAckType, CRC_confirm)))
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

	// la sonde 2 a une alerte haute et la sonde 1 reste a alerte basse			
	if(!perteConnexion && niveauAlerte1 == BAS && niveauAlerte2 == HAUT)
	{
		flagConfSonde1 = true;
		
		if(flagTimeOutEmission2)
			etatAlerteGlobal = EMISSION;
		else
			etatAlerteGlobal = AVERTISSEMENT;
	}
	else
	{
		flagConfSonde2 = false;	
		isCountingAttenteSonde2 = false;
		flagTimeOutEmission2 = false;		
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

// 
void LED_Toggle(void) {
	PORTD ^= 0x01; // change letat de la led branché sur la pin PD0
}

// analyse le message recu wireless et sort le tableau recu en eliminant lentete / gestion derreurs
bool recoieMessage(EtatAlerte message, AckType acktype, bool CRC_confirm)
{
	//////////
	//////////
	//////////
	// A FAIRE
	//////////
	//////////
	//////////
	
	return true;
}

// envoie le message par communication sans fils
void envoieMessage(EtatAlerte message, AckType acktype)
{
	//////////
	//////////
	//////////
	// A FAIRE
	//////////
	//////////
	//////////
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
		intervalLEDAlerte = 8000; // toggle a chaque 1 sec
	}
	else if(etatalerteglobal == AVERTISSEMENT) // allume leds bleu et eteint les autres
	{
		intervalLEDAlerte = 3000; // toggle a chaque 0.5 sec	
	}
	else if(etatalerteglobal == EMISSION) // allume leds rouge et eteint les autres
	{
		intervalLEDAlerte = 1000; // toggle a chaque 0.2 sec
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
		
	if((valeurPh < 7.0) && (valeurPh > 6.0)) // allume leds vert et eteint les autres
	{
		PORTB |= 0x40; // (ROUGE)
		PORTB |= 0x02; // (BLEU)
		PORTB &= 0xDF; // (VERT)
	}
	else if(valeurPh > 7.0) // allume leds bleu et eteint les autres
	{
		PORTB |= 0x40; // (ROUGE)
		PORTB &= 0xFD; // (BLEU)
		PORTB |= 0x20; // (VERT)
	}
	else if(valeurPh < 6.0) // allume leds rouge et eteint les autres
	{
		PORTB &= 0xBF; // (ROUGE)
		PORTB |= 0x02; // (BLEU)
		PORTB |= 0x20; // (VERT)
	}
	else
	{
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
		
	DDRB |= 0x20; //PB5 output (vert) (pour ph)
		
	DDRB |= 0x02; //PB1 output (bleu) (pour ph)
	
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



