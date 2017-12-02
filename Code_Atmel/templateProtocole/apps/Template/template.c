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

/*- Variables --------------------------------------------------------------*/
// Put your variables here
uint8_t receivedWireless;	//cette variable deviendra 1 lorsqu'un nouveau paquet aura �t� recu via wireless (et copi� dans "PHY_DataInd_t ind"
							//il faut la mettre a 0 apres avoir g�r� le paquet; tout message recu via wireless pendant que cette variable est a 1 sera jet�

PHY_DataInd_t ind; //cet objet contiendra les informations concernant le dernier paquet qui vient de rentrer


/*- Implementations --------------------------------------------------------*/

// Put your function implementations here

/*************************************************************************//**
*****************************************************************************/
bool genMessage(Alerte alerte, uint8_t *message[128])
{
	for(int i = 0; i < IDCodeLen ; i++))
	{
		*message[i] = IDCode[i];
	}
	
	*message[IDCodeLen] = 0;
	
	for (int i = 1; i <= 6; i++)
	{
		
	}
	
	if (alerte == ALERTE1)
	{

	}	
	else if(alerte == ALERTE2)
	{
		
	}
	else if(alerte == ALERTE3)
	{
		
	}
	else
	{
		return false;
	}
	
}


static void APP_TaskHandler(void)
{
  char receivedUart = 0;

  receivedUart = Lis_UART();  
  if(receivedUart)		//est-ce qu'un caractere a �t� recu par l'UART?
  {
	  Ecris_UART((char*)receivedUart);	//envoie l'echo du caractere recu par l'UART

	  if(receivedUart == 'a')	//est-ce que le caractere recu est 'a'? 
		{
		uint8_t demonstration_string[128] = "123456789A"; //data packet bidon
		genMessage(ALERTE1,demonstration_string);
		Ecris_Wireless(demonstration_string, 10); //envoie le data packet; nombre d'�l�ments utiles du paquet � envoyer
		}
  }




  
  if(receivedWireless == 1) //est-ce qu'un paquet a �t� recu sur le wireless? 
  {

//si quelqu'un a une m�thode plus propre / mieux int�gr�e � proposer pour faire des "printf" avec notre fonction Ecris_UART, je veux bien l'entendre! 
	
	
	Ecris_UART("\n\rnew trame! size: %d, RSSI: %ddBm\n\r contenu: %s\n\r", ind.size, ind.rssi, ind.data);
	
	receivedWireless = 0; 
	
	
/*
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
	
	*/
  }
}



/*************************************************************************//**
*****************************************************************************/
int main(void)
{
  SYS_Init();
   
  while (1)
  {
    PHY_TaskHandler(); //stack wireless: va v�rifier s'il y a un paquet recu
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
	
	char i = 0;
	while(buffer[i] != (char)0)					//tant qu'on n'est pas arriv� � la fin de la cha�ne de caract�res
	{
		UDR1 = buffer[i];						//transmet le caractere en cours
		while(!(UCSR1A & (0x01 << UDRE1)));		//attend que le caract�re soit fini d'envoyer
		i++;									//incr�menter l'index du caractere en cours
	}
	
	
	
	va_end(args);
	
}


void init_UART(void)
{
	//baud rate register = Fcpu / (16*baud rate - 1)
	//baudrate = 9600bps //s'assurer que la fuse CLKDIV8 n'est pas activ�e dans les Fuses, sinon ca donne 1200bps
	UBRR1H = 0x00;
	UBRR1L = 53;
	
	UCSR1A = 0x00;
	UCSR1B = 0x18; //receiver, transmitter enable, no parity
	UCSR1C = 0x06; //8-bits per character, 1 stop bit
}


