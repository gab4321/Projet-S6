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
#include "sys.h"
#include "phy.h"
#include <stdio.h>
#include <stdarg.h>
#include "header.h"
#include <string.h>
#include "libcrc-2.0/include/checksum.h"
/*- Definitions ------------------------------------------------------------*/
// Put your preprocessor definitions here

/*- Types ------------------------------------------------------------------*/
// Put your type definitions here

/*- Prototypes -------------------------------------------------------------*/
// Put your function prototypes here
char Lis_UART(void);
void Ecris_UART(char *data, ...);
void init_UART(void);
void SYS_Init(void);
int testeu = 45;

uint8_t testBuffer2[MessageLength];
Alerte testAlerte2 = BAS;
uint8_t ackTest2 = 1;
bool crcOK;
char *ptr;

/*- Variables --------------------------------------------------------------*/
// Put your variables here
uint8_t receivedWireless;	//cette variable deviendra 1 lorsqu'un nouveau paquet aura été recu via wireless (et copié dans "PHY_DataInd_t ind"
							//il faut la mettre a 0 apres avoir géré le paquet; tout message recu via wireless pendant que cette variable est a 1 sera jeté

PHY_DataInd_t ind; //cet objet contiendra les informations concernant le dernier paquet qui vient de rentrer


/*- Implementations --------------------------------------------------------*/

// Put your function implementations here

/*************************************************************************//**
*****************************************************************************/

bool genMessage(Alerte alerte, char *message, uint8_t ack)
{

   // formatage du Id mdp
	memcpy(message, IDCode, strlen(IDCode));

	// formatage du ack
	memcpy(message+strlen(IDCode), &ack, sizeof(ack));
	//message[strlen(IDCode)] = ack;
	
	
	// formatage du message
	 memcpy(message+strlen(IDCode)+sizeof(ack), &alerte, sizeof(alerte));

	uint8_t crcResult = crc_8((unsigned char*)message, MessageLength-1);

	//memcpy(message+(sizeof(message)- sizeof(crcResult)), &crcResult, sizeof(crcResult));
	memcpy(message+(MessageLength-1), &crcResult, sizeof(crcResult));


	//uint8_t crcResult2 = crc_8((unsigned char*)message, MessageLength);

	Ecris_UART("Message sent");

	return true; //a changer
	
}


bool decodeMessage(Alerte *alerte, char *message, uint8_t *ack, bool *crcConfirm)
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
bool AdressOK;

static void APP_TaskHandler(void)
{

  char receivedUart = 0;

  receivedUart = Lis_UART();  
  if(receivedUart)		//est-ce qu'un caractere a été recu par l'UART?
  {
	  Ecris_UART("Echo: %c", receivedUart);	//envoie l'echo du caractere recu par l'UART

	  if(receivedUart == 'a')	//est-ce que le caractere recu est 'a'? 
		{
		//uint8_t demonstration_string[MessageLength] = "123456789A"; //data packet bidon

		// variables pour test
		uint8_t testBuffer[MessageLength];
		Alerte testAlerte = HAUT;
		uint8_t ackTest = 2;
		

		//Bufftest[128] = "123456789A";
		//Ecris_Wireless(Bufftest, 10);
		
		genMessage(testAlerte, testBuffer, ackTest);
		Ecris_Wireless(testBuffer, 127);
		
		/*char testBuffer2[MessageLength];
		Alerte testAlerte2 = BAS;
		uint8_t ackTest2 = 1;
		bool crcOK;
		
		AdressOK = false;*/
		
		//AdressOK = decodeMessage(&testAlerte2, testBuffer, &ackTest2, &crcOK);

		//Ecris_Wireless(demonstration_string, 10); //envoie le data packet; nombre d'éléments utiles du paquet à envoyer
		}
  }




  
  if(receivedWireless == 1) //est-ce qu'un paquet a été recu sur le wireless? 
  {

//si quelqu'un a une méthode plus propre / mieux intégrée à proposer pour faire des "printf" avec notre fonction Ecris_UART, je veux bien l'entendre! 
	
	
	bool AdresseCorrect;
	
	char i = 0;
	/*
	ptr = ind.data;
	while (i< ind.size)
	{
		testBuffer2[i] = *ptr++;
		i++;
	}
	*/
	
	sprintf(testBuffer2, "%s" , ind.data);
	
	AdresseCorrect = decodeMessage(&testAlerte2, testBuffer2, &ackTest2, &crcOK);
	
	//Ecris_UART("\n\rnew trame! size: %d, RSSI: %ddBm\n\r Data: %s\n\r", ind.size, ind.rssi, testBuffer2);
	
	if(AdresseCorrect)
	{
		Ecris_UART("\n\rnew trame! size: %d, RSSI: %ddBm\n\r alerte: %d\n\r Ack: %d\n\r CRC :%d", ind.size, ind.rssi, testAlerte2,ackTest2, crcOK);
	}
	
	receivedWireless = 0; 
	
  }
}



/*************************************************************************//**
*****************************************************************************/
int main(void)
{
  SYS_Init();
   
  while (1)
  {
    PHY_TaskHandler(); //stack wireless: va vérifier s'il y a un paquet recu
    APP_TaskHandler(); //l'application principale roule ici
  }
}






//FONCTION D'INITIALISATION
/*************************************************************************//**
*****************************************************************************/
void SYS_Init(void)
{
receivedWireless = 0;
wdt_disable(); 
init_UART();
PHY_Init(); //initialise le wireless
PHY_SetRxState(1); //TRX_CMD_RX_ON
}
//




















//FONCTIONS POUR L'UART

char Lis_UART(void)
{

char data = 0; 


	if(UCSR1A & (0x01 << RXC1))
	{
		data = UDR1;
	}
	
return data;
}


void Ecris_UART(char *data, ...)
{
	char buffer[256];
	va_list args;
	va_start (args, data);
	vsprintf(buffer, data, args);				//cree une chaine de caracteres en formattant les arguments en entree
	
	uint8_t i = 0;
	while(buffer[i] != (char)0)					//tant qu'on n'est pas arrivé à la fin de la chaîne de caractères
	{
		UDR1 = buffer[i];						//transmet le caractere en cours
		while(!(UCSR1A & (0x01 << UDRE1)));		//attend que le caractère soit fini d'envoyer
		i++;									//incrémenter l'index du caractere en cours
	}
	
	
	
	va_end(args);
	
}


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