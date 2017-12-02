/*
 * code de test des fonctions du projet S6
 * Gabriel Guilmain
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

void ADC_setup(void);
void LED_setup(void);
void Board_Init(void);
void Timer_Init(void);
void start_ADC(void);

void printf_float(float message, int precision);
void printf_int(int message);
void printf_string(char message[100]);
void printf_func(char* buf);

char Lis_UART(void);
void Ecris_UART(char data);
void init_UART(void);

void wdt_disable(void);
void set_couleur(int rouge, int bleu, int vert);
float lecture_ADC(void);
float conv_PH(float adc_value);

volatile int compteur_color = 0;
volatile long int compteur_test = 0;
volatile long int compteur_test2 = 0;
volatile int valeur_rouge = 0;
volatile int valeur_vert = 0;
volatile int valeur_bleu = 0;
volatile int flag_ADC = 0;
volatile int flag_test = 0;
volatile int flag_test2 = 0;
volatile int testt = 0;

int ii,jj,kk;
float voltage_adc = 0;
float PH_value = 0;

unsigned int test1 = 0;
unsigned int test2 = 0;

unsigned int ADC_value = 0;
int ADC_value_low = 0;
int ADC_value_high = 0;

// ISR du timer 1: set le pwm des couleurs des LEDs
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
	
	
	if(compteur_test2++ >= 10)
	{
		compteur_test2 = 0;
		flag_test2 = 1;
	}
	
	
	//flag_test2 = 1;
}

// ISR du ADC
ISR(ADC_vect) 
{
	ADCSRA &= 0b11101111; // disable le flag de lADC (vraiment utile ?)
	flag_ADC = 1; // flag pour enabler la lecture dans le main
}

// FONCTION MAIN
int main(void)
{
	
	// Initialisation du board
	Board_Init();
	Timer_Init();
	ADC_setup();
	
	// Enable global interrupts
	sei();
	
	// Boucle principale
	while(1)
	{
		// Test des LEDS RGB
		// jaune
		//valeur_rouge = 85;
		//valeur_vert = 15;
		//valeur_bleu = 1;
		
		
		// conversion ADC
		if(flag_test == 1)
		{	
			flag_test = 0;
			
			start_ADC(); // start ADC convertion
		}

		/*
		// lecture ADC
		if(flag_ADC == 1)
		{
			flag_ADC = 0;
			
			voltage_adc = lecture_ADC();
			
			char buf[196];
			sprintf( buf, "Tension de l'ADC: %f V\n",voltage_adc);
			printf_func(buf);
			
			PH_value = conv_PH(voltage_adc);
			
			sprintf( buf, "Valeur du pH: %f \n",PH_value);
			printf_func(buf);
		}
		*/
		
		if(flag_test2 == 1)
		{
			flag_test2 = 0;
			char buf[196];
			sprintf( buf, "test timer lent: 5 secondes\n");
			printf_func(buf);
		}
		
	}
}

float conv_PH(float adc_value)
{
	float PHH = 0;
	
	PHH = adc_value*(6.8+3.3)/6.8*3.5; // pour prendre en compte diviseur resistif
	
	return PHH;
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

// INIT BOARD
void Board_Init(void) 
{
	cli();
	wdt_disable();
	init_UART();
	LED_setup();
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

void wdt_disable(void) // cest utile ??
{
	// Disable watchdog timer
	asm("wdr");
	MCUSR = 0;
	WDTCSR |= (1 << WDCE) | (1 << WDE);
	WDTCSR = 0x00;
}

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

void printf_func(char* buf)
{
	char *ptr = buf;
	while( *ptr != (char)0 )
		Ecris_UART( *ptr++ );
}

void printf_float(float message, int precision)
{
	unsigned int decimal;
	unsigned int entier;
	int i = 0;
	
	// test pour séparer les décimals et les entiers pour faire le printf
	
	if(message < 0)
		entier = floor(message * -1.0)*-1.0;
	else
		entier = floor(message);
		
	char buf[196];
	sprintf( buf, "%d.",entier);
	char *ptr = buf;
	while( *ptr != (char)0 )
		Ecris_UART( *ptr++ );
	
	for(i = 1; i < precision; i++)
	{
		if(message < 0)
			decimal = ((message - ceil(message))*-1)*pow(10,i);
		else
			decimal = (message - floor(message))*(double)(pow(10,i));
			
		if(decimal != 0)
			break;
		
		sprintf( buf, "%u",decimal);
		ptr = buf;
		while( *ptr != (char)0 )
			Ecris_UART( *ptr++ );
	}
	
	if(message < 0)
		decimal = round(((message - ceil(message))*-1)*pow(10,precision));
	else
		decimal = round((message - floor(message))*(double)(pow(10,precision)));
	
	sprintf( buf, "%u",decimal);
	ptr = buf;
	while( *ptr != (char)0 )
		Ecris_UART( *ptr++ );
	/*
	sprintf( buf, "\n");
	ptr = buf;
	while( *ptr != (char)0 )
	Ecris_UART( *ptr++ );
	*/
}

void printf_int(int message)
{
	char buf[196];
	sprintf( buf, "%d\n",message);
	char *ptr = buf;
	while( *ptr != (char)0 )
		Ecris_UART( *ptr++ );
}

void printf_string(char message[100])
{
	char *ptr = message;
	while( *ptr != (char)0 )
		Ecris_UART( *ptr++ );
}

void start_ADC()
{
	ADCSRA |= 0b01000000;
}


