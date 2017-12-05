/*
 * header.h
 *
 * Created: 2017-12-02 14:55:55
 *  Author: faff2302
 */ 


#ifndef TEMPLATE_H_
#define TEMPLATE_H_

/*- Includes ---------------------------------------------------------------*/
#include <string.h>
#include "libcrc-2.0/include/checksum.h"
#include "sys.h"
#include "phy.h"
#include <stdio.h>
#include <stdarg.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <math.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>



/*- Definitions ------------------------------------------------------------*/
#define IDCode "uWLmpJ9elFpnWre3MwBHzDAUtjWSYoa4O5SpdDtNGr9cneVomBA5B24qZJHA9phhcm7mMKBxizZjtNoiNLIciWRyCoUNfW0c78FM" 
#define MessageLength 103
#define PH_ERROR_CODE 0xFFFF

/*- Enum ------------------------------------------------------------*/
typedef enum EtatAlerte EtatAlerte;
enum EtatAlerte
{
	// niveaux dalerte
	BAS,
	HAUT,
	INDETERMINE,
	ERROR_ALERTE
};

typedef enum EtatComm EtatComm;
enum EtatComm
{
	// etat de communication
	IDLE,
	ATTENTE_ACK1,
	ATTENTE_ACK2
};

typedef enum AckType AckType;
enum AckType
{
	// type dacknowledge
	POLL,
	ACK1,
	ACK2,
	ERROR_ACK
};

typedef enum EtatAlerteGlobal EtatAlerteGlobal;
enum EtatAlerteGlobal
{
	// niveaux dalerte global pour lemission ou non de pH
	ATTENTE,
	AVERTISSEMENT,
	EMISSION
};

/*- Prototypes -------------------------------------------------------------*/
// Put your function prototypes here
char Lis_UART(void);
void Ecris_UART(char *data, ...);
void init_UART(void);
void SYS_Init(void);

// Put your function prototypes here
void start_ADC(void);
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

void envoieMessage(EtatAlerte alerte, AckType acktype);
bool recoieMessage(EtatAlerte *alerte, AckType *acktype, bool *CRC_confirm);
bool genMessage(EtatAlerte alerte, char *message, AckType ack);
bool decodeMessage(EtatAlerte *alerte, char *message, AckType *ack, bool *crcConfirm);

/*- Variables --------------------------------------------------------------*/
uint8_t receivedWireless;	
PHY_DataInd_t ind; 

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

EtatAlerte niveauAlerte1 = BAS;
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

uint8_t messageBuffer[MessageLength];







#endif /* TEMPLATE_H_ */