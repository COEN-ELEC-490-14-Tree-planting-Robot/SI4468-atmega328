/*
 * Project: Si4463 Radio Library for AVR and Arduino (Ping server example)
 * Author: Zak Kemble, contact@zakkemble.co.uk
 * Copyright: (C) 2017 by Zak Kemble
 * License: GNU GPL v3 (see License.txt)
 * Web: http://blog.zakkemble.co.uk/si4463-radio-library-avr-arduino/
 */

/*
 * Ping server
 *
 * Listen for packets and send them back
 */
#define F_CPU 8000000
#define BAUD 56000

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <util/atomic.h>
#include <util/setbaud.h>
#include <util/delay.h>
#include <stdio.h>
#include "Si446x.h"

#define CHANNEL 20
#define MAX_PACKET_SIZE 8

#define PACKET_NONE		0
#define PACKET_OK		1
#define PACKET_INVALID	2

#define USART_RX_BUFFER_SIZE 32    //2,4,8,16,32,64,128 or 256 bytes 
#define USART_TX_BUFFER_SIZE 256     //2,4,8,16,32,64,128 or 256 bytes
#define USART_RX_BUFFER_MASK (USART_RX_BUFFER_SIZE - 1)
#define USART_TX_BUFFER_MASK (USART_TX_BUFFER_SIZE - 1)

#if (USART_RX_BUFFER_SIZE & USART_RX_BUFFER_MASK)
#error RX buffer size is not a power of 2
#endif
#if (USART_TX_BUFFER_SIZE & USART_TX_BUFFER_MASK)
#error TX buffer size is not a power of 2
#endif

static unsigned char USART_RxBuf[USART_RX_BUFFER_SIZE];
static volatile unsigned char USART_RxHead;
static volatile unsigned char USART_RxTail;
static unsigned char USART_TxBuf[USART_TX_BUFFER_SIZE];
static volatile unsigned char USART_TxHead;
static volatile unsigned char USART_TxTail;


typedef struct{
	uint8_t ready;
	int16_t rssi;
	uint8_t length;
	uint8_t buffer[MAX_PACKET_SIZE];
} pingInfo_t;

static int put(char c, FILE* stream)
{
	if(c == '\n')
		put('\r', stream);
	loop_until_bit_is_set(UCSR0A, UDRE0);
	UDR0 = c;
	return 0;
}

static FILE uart_io = FDEV_SETUP_STREAM(put, NULL, _FDEV_SETUP_WRITE);
static volatile pingInfo_t pingInfo;


void SI446X_CB_RXCOMPLETE(uint8_t length, int16_t rssi)
{
	// Make sure packet data will fit our buffer
	if(length > MAX_PACKET_SIZE)
		length = MAX_PACKET_SIZE;
	pingInfo.ready = PACKET_OK;
	pingInfo.rssi = rssi;
	pingInfo.length = length;
	Si446x_read((uint8_t*)pingInfo.buffer, length);
	//puts_P(PSTR("Received"));
}

void SI446X_CB_RXINVALID(int16_t rssi)
{
	pingInfo.ready = PACKET_INVALID;
	pingInfo.rssi = rssi;
	//puts_P(PSTR("Invalid"));
	// Radio will now be in idle mode
}

void main(void)
{
	clock_prescale_set(clock_div_1);

	// UART
	//PORTD |= _BV(PORTD0);
	//DDRD |= _BV(DDD1);
	USART_0_init();
	stdout = &uart_io;
	// Start up
	Si446x_init();
	Si446x_setTxPower(SI446X_MAX_TX_POWER);
	// Interrupts on
	sei();
	// Put into receive mode
	Si446x_RX(CHANNEL);
	
	uint32_t pings = 0;
	uint32_t invalids = 0;
	
	uint8_t size=0;
	uint8_t buff[MAX_PACKET_SIZE];
	USART_RxHead=-1;
	USART_RxTail=0;
	USART_TxTail=0;
	USART_TxHead=0;
	si446x_info_t data;
	Si446x_getInfo(&data);
	printf_P(PSTR("Part: %x\n"), data.part);
	puts_P(PSTR("Server start"));
	while(1)
	{
		// Wait for data
		while(pingInfo.ready == PACKET_NONE);
		if(pingInfo.ready != PACKET_OK){
			pingInfo.ready = PACKET_NONE;
			Si446x_RX(CHANNEL);
		}
		else
		{
			pingInfo.ready = PACKET_NONE;
			if( strcmp(pingInfo.buffer, "ACK") != 0)
			{
				USART_0_write_block(pingInfo.buffer,sizeof(pingInfo.buffer));
				char data[MAX_PACKET_SIZE] = {0};
				sprintf_P(data, PSTR("ACK"));
				Si446x_TX((uint8_t*)data,sizeof(data), CHANNEL, SI446X_STATE_RX);
			}
			//else
				//puts_P(PSTR("Received ACK"));
			Si446x_RX(CHANNEL);
		}
		
	}
}

ISR(USART_RX_vect)
{
	unsigned char data;
	unsigned char tmphead;
	
	//Read the received data
	data = UDR0;
	//Process data
	//USART_0_write(data);
	tmphead = (USART_RxHead + 1) & USART_RX_BUFFER_MASK;
	USART_RxHead = tmphead;
	if (data == ']'){
		uint8_t sent_data[tmphead+1];
		memcpy(&sent_data,&USART_RxBuf,tmphead+1);
		sent_data[tmphead]=']';
		Si446x_TX((uint8_t*)sent_data,tmphead+1, CHANNEL, SI446X_STATE_RX);
		USART_RxHead=-1;
	}
	else
		USART_RxBuf[tmphead] = data;
}

ISR(USART_UDRE_vect)
{
	unsigned char tmptail;
	//Check if all data is transmitted 
	if (USART_TxHead != USART_TxTail) {
		//Calculate buffer index
		tmptail = (USART_TxTail + 1) & USART_TX_BUFFER_MASK;
		//Store new index
		USART_TxTail = tmptail;
		//Start transmission
		UDR0 = USART_TxBuf[tmptail];
		} else {
		//Disable UDRE interrupt
		UCSR0B &= ~(1<<UDRIE0);
	}
}