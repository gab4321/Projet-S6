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

/*- Definitions ------------------------------------------------------------*/
// Put your preprocessor definitions here
#define PH_ERROR_CODE 0xFFFF

//#define MESSAGE_ALERTE_HAUT "ACPDNERW" // a changer eventuellement !!!

/*- Types ------------------------------------------------------------------*/
// Put your type definitions here

/*- Prototypes -------------------------------------------------------------*/
// Put your function prototypes here
void start_ADC(void);
void SYS_Init(void);
void ADC_setup(void);
void LED_setup(void);
void Timer_Init(void);
void printf_func(char* buf);

void ledAlerte(uint8_t etatAlerte);
void ledPH(float valeurPh);

void set_couleur(int rouge, int bleu, int vert);
float lecture_ADC(void);
float conv_PH(float adc_value);

void envoieMessage(uint8_t message, uint8_t acktype);

uint8_t recoieMessage(uint8_t message, uint8_t acktype);

char Lis_UART(void);
void Ecris_UART(char data);
void init_UART(void);
//void wdt_disable(void);



/*- Variables --------------------------------------------------------------*/
// Put your variables here
uint8_t receivedWireless;	//cette variable deviendra 1 lorsqu'un nouveau paquet aura été recu via wireless (et copié dans "PHY_DataInd_t ind"
							//il faut la mettre a 0 apres avoir géré le paquet; tout message recu via wireless pendant que cette variable est a 1 sera jeté

PHY_DataInd_t ind; //cet objet contiendra les informations concernant le dernier paquet qui vient de rentrer

// enum 
enum // A FAIRE: mettre dautres valeurs a lenum ?
{
	// niveaux dalerte
	BAS,
	HAUT,
	INDETERMINE,
	
	// etat de communication
	IDLE,
	ATTENTE_ACK1,
	ATTENTE_ACK2,
	
	// type dacknowledge
	POLL,
	ACK1,
	ACK2
};

// enum pour reussite ou echec de la reception
enum receptState
{
	SUCCESS,
	FAILED
};

receptState test = SUCCESS;

volatile int compteurMesurePh = 0;
volatile int compteurAttenteConf = 0;
volatile int compteurAttenteEmission = 0;
volatile int compteurAttenteAck1 = 0;
volatile int compteurAttenteAck2 = 0;
volatile int compteur_color = 0;
volatile int compteurPoll = 0;

volatile uint8_t intervalMesurePh = 2; // secondes
volatile uint8_t intervalAttenteAlerte = 30; // secondes
volatile uint8_t intervalAttenteEmission = 30; // secondes

volatile bool flagPh = false;
volatile bool flag_ADC = false;
volatile bool flagCancelAlerte = false;
volatile bool flagAttenteAlerte = false;
volatile bool flagAttenteEmission = false;
volatile bool flagEmissionFinie = false;
volatile bool flagPoll = false;
volatile bool flagAttenteAck1 = false;
volatile bool flagAttenteAck2 = false;
volatile bool flagTimeOutAck1 = false;
volatile bool flagTimeOutAck2 = false;

volatile int intervalPoll = 20; // secondes
volatile int TimeOutComm = 5; // secondes

uint8_t niveauAlerte1 = INDETERMINE;
uint8_t tempNiveauAlerte2 = INDETERMINE;
uint8_t niveauAlerte2 = INDETERMINE;

uint8_t etatEchangeEnvoie = IDLE;

uint8_t etatEchangeRecept = IDLE;

uint8_t receptAckType = INDETERMINE;

bool mesureStanby = false;
float valeurADC = 0;
float valeurPh = 7.0; // initialement PH neutre
uint8_t seuilPh = 4; // seuil du niveau 
uint8_t messageWireless[128];
uint8_t NbAlertesEnvoyee = 0;

////////////////////////////////////////////////////////////////////////////////
// INTERRUPT SERVICE ROUTINE
////////////////////////////////////////////////////////////////////////////////

// ISR DU TIMER 1
ISR(TIMER1_COMPA_vect) {
	
	compteur_color++; // test pour fonction de leds...
	
	// tests leds RGB
	//set_couleur(valeur_rouge, valeur_bleu, valeur_vert);
}

// ISR du timer 3: survient a chaque seconde -> pour timer les evenements de transmission & detection des sondes
ISR(TIMER3_COMPA_vect) {
	
	// compteur pour interval de mesure du pH
	if(compteurMesurePh++ > intervalMesurePh)
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
	
	// compteur de timeout de la comm si ne recoit pas dacknowledge UN (1) dans les delai requis
	if(compteurAttenteAck1++ > TimeOutComm && flagAttenteAck1)
	{
		compteurAttenteAck1 = 0;
		flagAttenteAck1 = false; 
		flagTimeOutAck1 = true;
	}
	else if(!flagAttenteAck1)
	{
		compteurAttenteAck1 = 0;
		flagTimeOutAck1 = false;
	}
	
	// compteur de timeout de la comm si ne recoit pas dacknowledge DEUX (2) dans les delai requis
	if(compteurAttenteAck2++ > TimeOutComm && flagAttenteAck2)
	{
		compteurAttenteAck2 = 0;
		flagAttenteAck2 = false; 
		flagTimeOutAck2 = true;
	}
	else if(!flagAttenteAck2)
	{
		compteurAttenteAck2 = 0;
		flagTimeOutAck2 = false;
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
	// Update les leds de niveau dalerte
	ledAlerte(1); // A FAIRE: METTRE BONNE VARIABLE
	
	// Update les leds de niveau de pH
	ledPH(valeurPh);
	
	// Part la convertion dADC a chaque 30 secondes pour updater letat de la sonde
	if(flagPh)
	{
		flagPh = false;
		
		start_ADC();
	}

	// mesure ADC terminée: on calcul la valeur du pH et on update letat de la presente sonde
	if(flag_ADC)
	{
		flag_ADC = false;
		
		valeurADC = lecture_ADC();
		
		valeurPh = conv_PH(valeurADC); 
	
		
		// verifie le niveau de PH
		if(valeurPh == PH_ERROR_CODE)
		{
			printf_func("ATTENTION: Valeur du pH non valide");	 // debug
		}
		else if(valeurPh < seuilPh)
		{
			printf_func("detection pH trop faible: Alerte HAUT"); // debug
			
			if(niveauAlerte1 == BAS) // si changement detat on envoie un poll a lautre sonde
			{
				flagPoll = true;
			}
			
			niveauAlerte1 = HAUT; 
		}	
		else if(valeurPh > seuilPh)
		{
			printf_func("detection pH OK: Alerte BAS"); // debug
			
			if(niveauAlerte1 == HAUT) // si changement detat on envoie un poll a lautre sonde
			{
				flagPoll = true;
			}
			
			niveauAlerte1 = BAS; 		
		}
	}
	
	// Part un envoi de letat dalerte à lautre sonde ce qui est en meme temps un poll pour letat dalerte
	// de cette deuxieme sonde
	if(flagPoll)
	{
		flagPoll = false;
		
		envoieMessage(niveauAlerte1, POLL); // effectue le checksum + code dentete + envoie du poll
		
		etatEchangeEnvoie = ATTENTE_ACK1; // etat de lechange sans fil	
		
		flagAttenteAck1 = true; // part le compteur qui attend de ack de lautre sonde
	}
	
	// Si la sonde 2 na pas repondu dans linterval de timeout pour le ack1 
	if(flagTimeOutAck1)
	{
		// A TERMINER -> ajouté un reenvoie et un compteur (apres x reenvoit on cancel)
		
		flagTimeOutAck1 = false;
		
		etatEchangeEnvoie = IDLE;
		
		niveauAlerte2 = INDETERMINE;
	}
	
	// Si la sonde 2 na pas repondu dans linterval de timeout pour le ack1
	if(flagTimeOutAck2)
	{
		// A TERMINER -> ajouté un reenvoie et un compteur (apres x reenvoit on cancel)
		
		flagTimeOutAck2 = false;
		
		etatEchangeEnvoie = IDLE;
			
		niveauAlerte2 = INDETERMINE;	
	}
	
	// si un paquet est recu sur le wireless
	if(receivedWireless == 1)
	{
		receivedWireless = 0;
		
		if(recoieMessage(tempNiveauAlerte2, receptAckType) == 1)
		{
			// si le ack type est poll -> envoie letat de la sonde avec ack1 en param
			// A FAIRE
			
			// si le ack type est ack1 -> update le niveau dalerte de la sonde 2 et envoie ack2 (Valide si etatEchangeEnvoie = ATTENTE_ACK1)
			// A FAIRE
			
			// si le ack type est ack2 -> on arrete le compteur qui calcul le timeout avant de faire un reenvoie de letat de la presente sonde a la sonde 2
			// lechange a donc eu lieu completement (Valide si etatEchangeEnvoie = ATTENTE_ACK2)
			// A FAIRE
			
			// si le type de ack recu de la sonde 2 ne correspond pas a letat dattente de la sonde 1 -> ne rien faire avec ce message !!		
		}
		if(recoieMessage(tempNiveauAlerte2, receptAckType) == 2)
		{
			// IL FAUT DEMANDER UN REENVOIT...
			
			// 1 -> si (etatEchangeEnvoie = ATTENTE_ACK1) on revenvoit letat de la sonde 1 avec le ack a POLL
			
			// 2 -> si (etatEchangeEnvoie = ATTENTE_ACK2) on revenvoit letat de la sonde 1 avec le ack a ACK1
			
			// 3 -> si (etatEchangeEnvoie = IDLE) la sonde 1 nattendait aucun message particulier: soit on implemente un message special
			// on peut soit ignorer et attendre au prochain poll
			
		}	
	}	
	
	////////////////////////////////////////////////////////////////////////////////
	// ICI: METTRE UNE FONCTION QUI COMPUTE SI ON EMET DES ULTRASONS OU NON
	// SELON LES NIVEAU DALERTES COMBINÉS 
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

// analyse le message recu wireless et sort le tableau recu en eliminant lentete / gestion derreurs
uint8_t recoieMessage(uint8_t message, uint8_t acktype)
{
	//////////
	//////////
	//////////
	// A FAIRE
	//////////
	//////////
	//////////
	
	// pour debug...
	////////////////////////////////////////////////////////////////////////////////
	/*
	char buf[196];
	char *ptr = buf;
		
	sprintf( buf, "\n\rnew trame! size: %d, RSSI: %ddBm\n\r", ind.size, ind.rssi );
	printf_func(buf);
		
	printf_func("contenu: ");

	ptr = ind.data;
	char i = 0;
	while( i < ind.size )
	{
		Ecris_UART( *ptr++ );
		i++;
	}

	sprintf( buf, "\n\r");
	ptr = buf;
	while( *ptr != (char)0 )
	Ecris_UART( *ptr++ );
		
	receivedWireless = 0;
	*/
	////////////////////////////////////////////////////////////////////////////////
}

// envoie le message par communication sans fils
void envoieMessage(uint8_t message, uint8_t acktype)
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
void ledAlerte(uint8_t etatAlerte)
{
	//////////
	//////////
	//////////
	// A FAIRE
	//////////
	//////////
	//////////
}

// gere les leds du niveau de pH
void ledPH(float valeurPh)
{
	//////////
	//////////
	//////////
	// A FAIRE
	//////////
	//////////
	//////////
}

//FONCTION D'INITIALISATION
void SYS_Init(void)
{
	cli();
		
	receivedWireless = 0;
	wdt_disable(); 
	init_UART();
	PHY_Init(); //initialise le wireless
	PHY_SetRxState(1); //TRX_CMD_RX_ON
	
	//wdt_disable();
	LED_setup();
	ADC_setup();
	Timer_Init();	
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
	// init ports des LEDs (port B)
	DDRB = 0x00;
		
	DDRB |= 0x40; //PB6 output (rouge)
		
	DDRB |= 0x20; //PB5 output (vert)
		
	DDRB |= 0x02; //PB1 output (bleu)
		
	PORTB = 0xFF; //LEDs a off
	
	///////////////////////////////////////////
	// AUTRES PORTS A FAIRE POUR STRIPE DE LED 2
	///////////////////////////////////////////
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

// FONCTION UTILISÉ POUR PRINF
void printf_func(char* buf)
{
	char *ptr = buf;
	while( *ptr != (char)0 )
	Ecris_UART( *ptr++ );
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

// ecrit uart pour printf
void Ecris_UART(char data)
{
	UDR1 = data;
	while(!(UCSR1A & (0x01 << UDRE1)));
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

/*
// NE SAIS PAS CE QUE CA FAIT: A VOIR ***
void wdt_disable(void) // cest utile ??
{
	// Disable watchdog timer
	asm("wdr");
	MCUSR = 0;
	WDTCSR |= (1 << WDCE) | (1 << WDE);
	WDTCSR = 0x00;
}
*/



