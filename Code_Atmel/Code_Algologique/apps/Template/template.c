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
void ledAlerte(uint8_t niveau);

char Lis_UART(void);
void Ecris_UART(char data);
void init_UART(void);
//void wdt_disable(void);

void set_couleur(int rouge, int bleu, int vert);
float lecture_ADC(void);
float conv_PH(float adc_value);

/*- Variables --------------------------------------------------------------*/
// Put your variables here
uint8_t receivedWireless;	//cette variable deviendra 1 lorsqu'un nouveau paquet aura été recu via wireless (et copié dans "PHY_DataInd_t ind"
							//il faut la mettre a 0 apres avoir géré le paquet; tout message recu via wireless pendant que cette variable est a 1 sera jeté

PHY_DataInd_t ind; //cet objet contiendra les informations concernant le dernier paquet qui vient de rentrer

//////////////////////////////////////////////////
// VARIABLES DE TEST
volatile int compteur_color = 0;
volatile long int compteur_test = 0;
volatile int valeur_rouge = 0;
volatile int valeur_vert = 0;
volatile int valeur_bleu = 0;
volatile int flag_test = 0;
volatile int flag_test2 = 0;
volatile int testt = 0;
volatile int compteur_test2 = 0;

int ii,jj,kk;
float voltage_adc = 0;
float PH_value = 0;

unsigned int test1 = 0;
unsigned int test2 = 0;

unsigned int ADC_value = 0;
int ADC_value_low = 0;
int ADC_value_high = 0;

//////////////////////////////////////////////////
// VRAI VARIABLES

uint8_t niveauAlerte = 1;
volatile int compteurMesurePh = 0;
volatile uint8_t intervalMesurePh = 2; // secondes
volatile bool flagPh = false;
volatile bool flag_ADC = false;
float valeurADC = 0;
float valeurPh = 0;
uint8_t seuilPh = 4; // seuil du niveau 

////////////////////////////////////////////////////////////////////////////////
// INTERRUPT SERVICE ROUTINE
////////////////////////////////////////////////////////////////////////////////

// ISR DU TIMER 1
ISR(TIMER1_COMPA_vect) {
	
	compteur_color++;
	compteur_test++;
	
	// start ADC conversion each X seconds
	if(compteur_test >= 100000)
	{
		compteur_test = 0;
		flag_test = 1;
	}
	
	// tests leds RGB
	//set_couleur(valeur_rouge, valeur_bleu, valeur_vert);
	
}

// ISR du timer 3: survient a chaque seconde -> pour timer les evenements de transmission & detection des sondes
ISR(TIMER3_COMPA_vect) {
	
	// 
	if(compteurMesurePh++ > intervalMesurePh)
	{
		compteurMesurePh = 0;
		flagPh = true;
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
	// update les leds de niveau dalerte
	ledAlerte(niveauAlerte);
	
	// mesure du PH en boucle
	////////////////////////////////////////////////////////////////////////////////
	if(flagPh)
	{
		flagPh = false;
		start_ADC();
	}

	if(flag_ADC)
	{
		flag_ADC = false;
		
		valeurADC = lecture_ADC();
		
		
		
		valeurPh = conv_PH(valeurADC);
		
		if (valeurPh == PH_ERROR_CODE)
		{
			printf_func("ATTENTION: Valeur du pH non valide");		
		}		
	}
	////////////////////////////////////////////////////////////////////////////////
	
	// verifie le niveau de PH
	////////////////////////////////////////////////////////////////////////////////
	if(valeurPh < seuilPh)
	{
		niveauAlerte = 2; // 
		
	}
	
	
	
	
	
	////////////////////////////////////////////////////////////////////////////////
	
	
	
	
	
	
	
	
	
	
	
	
	
	/*
  char receivedUart = 0;

  receivedUart = Lis_UART();  
  if(receivedUart) //est-ce qu'un caractere a été recu par l'UART?
  {
	  Ecris_UART(receivedUart);	//envoie l'echo du caractere recu par l'UART

	  if(receivedUart == 'a')	//est-ce 
	  .que le caractere recu est 'a'? 
		{
		uint8_t demonstration_string[128] = "123456789A"; //data packet bidon
		Ecris_Wireless(demonstration_string, 10); //envoie le data packet; nombre d'éléments utiles du paquet à envoyer
		}
  }
  */
  
  /*
  if(receivedWireless == 1) //est-ce qu'un paquet a été recu sur le wireless? 
  {
	char buf[196];

	//si quelqu'un a une méthode plus propre / mieux intégrée à proposer pour faire des "printf" avec notre fonction Ecris_UART, je veux bien l'entendre! 
	sprintf( buf, "\n\rnew trame! size: %d, RSSI: %ddBm\n\r", ind.size, ind.rssi );
	char *ptr = buf;
	while( *ptr != (char)0 )
		Ecris_UART( *ptr++ );
		
	sprintf( buf, "contenu: ");
	ptr = buf;
	while( *ptr != (char)0 )
		Ecris_UART( *ptr++ );

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
  }
  */
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

// gere les leds du niveau dalerte
void ledAlerte(uint8_t niveau)
{
	// A FAIRE
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

// FONCTION QUI EFFECTUE LA LECTURE DE LADC
float lecture_ADC(void)
{
	float volt = 0;
	float Offset_ADC = 0.1;
	
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



