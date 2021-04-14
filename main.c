#include <atmel_start.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

/*
#define ERROR 0x46
#define SUCCESS 0x50
#define PACKET_NONE		0
#define PACKET_OK		1
#define PACKET_INVALID	2
#define CHANNEL 0
#define MAX_PACKET_SIZE 10
typedef struct{
	uint8_t ready;
	int16_t rssi;
	uint8_t length;
	uint8_t buffer[MAX_PACKET_SIZE];
} pingInfo_t;

#define USART_RX_BUFFER_SIZE t#define USART_TX_BUFFER_SIZE 64     
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
static volatile bool needclear=false;


void USART1_Init(unsigned int ubrr_val);
unsigned char USART1_Receive(void);
void USART1_Transmit(unsigned char data);
static uint8_t Interrupt_vector[9];
static uint8_t cliflag;
static uint8_t debugging=0;

int main_station(void)
{
	uint8_t RX_buff[64];
	SI4468_RX(CHANNEL);
	while (1)
	{
		USART_0_write(SI4468_GetState());
		if(needclear){
			USART_0_write(0x11);
			SI4468_Clear_All_Interrupt(Interrupt_vector);
			needclear=false;
			EIMSK |= (1<<INT0);
		}
		else if(USART_RxTail!=USART_RxHead){
			USART_0_write(0x12);
			uint8_t size;
			uint8_t buff[USART_RX_BUFFER_SIZE];
			if(USART_RxHead>USART_RxTail){
				size = USART_RxTail-USART_RxHead;
				memcpy(buff,&USART_RxBuf[USART_RxTail],size);
			}
			else{
				size= (USART_RX_BUFFER_SIZE-USART_RxTail);
				memcpy(buff,&USART_RxBuf[USART_RxTail],size);
				memcpy(&buff[size],&USART_RxBuf[0],USART_RxHead+1);
				size+=USART_RxHead;
			}
			//USART_0_write_block(buff,size);
			PB2_set_level(0);
			SI4468_TX(buff,size,CHANNEL,SI446X_STATE_RX);
			PB2_set_level(1);
			USART_RxTail=USART_RxHead;
		}
		_delay_ms(3000);
	}
}

int main_turtlebot(void)
{
	//USART_0_write(SI4468_TX(1));
	USART_0_write(SI4468_TX("Hello World",sizeof("Hello World"),CHANNEL,SI446X_STATE_RX));
	while (1)
	{
		USART_0_write(SI4468_GetState());
		if(needclear){
			USART_0_write(0x11);
			SI4468_Clear_All_Interrupt(Interrupt_vector);
			needclear=false;
			EIMSK |= (1<<INT0);
		}
		else if(USART_RxTail!=USART_RxHead){
			USART_0_write(0x12);
			uint8_t size;
			uint8_t buff[USART_RX_BUFFER_SIZE];
			if(USART_RxHead>USART_RxTail){
				size = USART_RxTail-USART_RxHead;
				memcpy(buff,&USART_RxBuf[USART_RxTail],size);
			}
			else{
				size= (USART_RX_BUFFER_SIZE-USART_RxTail);
				memcpy(buff,&USART_RxBuf[USART_RxTail],size);
				memcpy(&buff[size],&USART_RxBuf[0],USART_RxHead+1);
				size+=USART_RxHead;
			}
			//USART_0_write_block(buff,size);
			PB2_set_level(0);
			SI4468_TX(buff,size,CHANNEL,SI446X_STATE_RX);
			PB2_set_level(1);
			USART_RxTail=USART_RxHead;
		}
		_delay_ms(3000);
	}
}


int main(void)
{
	atmel_start_init();
	sei();
	USART_RxHead=0;
	USART_RxTail=0;
	USART_TxHead=0;
	USART_TxTail=0;
	
	//USART_0_write('b');
	//USART_0_write(SI4468_START_SEQUENCE());
	
	SI4468_INIT();
	USART_0_write(PD4_get_level());
	PD2_set_dir(PORT_DIR_IN);
	PD2_set_pull_mode(PORT_PULL_OFF);
	EIMSK |= (1<<INT0);
	EICRA = (0<<ISC01)|(0<<ISC00);
	//SI4468_Clear_All_Interrupt(NULL);
	
	
	//COM4
	//main_turtlebot();
	//COM5
	main_station();
	//
	
	
	while(1);
	while (1)
	{
		//USART_0_write(PD4_get_level());
		USART_0_write(SI4468_GetState());
		//USART_0_write(SI4468_WAITCTS());
		_delay_ms(3000);
	}
	//return 0;
}
*/

/*
ISR(USART_RX_vect)
{
	unsigned char data;
	unsigned char tmphead;

	data = UDR0;
	
	//Process data
	//USART_0_write(data);
	
	tmphead = (USART_RxHead + 1) & USART_RX_BUFFER_MASK;
	USART_RxHead = tmphead;
	
	if (tmphead == USART_RxTail) {
	}
	USART_RxBuf[tmphead] = data;
	
}

ISR(USART_UDRE_vect)
{
	unsigned char tmptail;
	if (USART_TxHead != USART_TxTail) {
		tmptail = (USART_TxTail + 1) & USART_TX_BUFFER_MASK;
		USART_TxTail = tmptail;
		UDR0 = USART_TxBuf[tmptail];
		} else {
		UCSR0B &= ~(1<<UDRIE0);
	}
}

unsigned char USART1_Receive(void)
{
	unsigned char tmptail;
	
	while (USART_RxHead == USART_RxTail);
	tmptail = (USART_RxTail + 1) & USART_RX_BUFFER_MASK;
	USART_RxTail = tmptail;
	return USART_RxBuf[tmptail];
}

void USART1_Transmit(unsigned char data)
{
	unsigned char tmphead;
	
	tmphead = (USART_TxHead + 1) & USART_TX_BUFFER_MASK;
	while (tmphead == USART_TxTail);
	USART_TxBuf[tmphead] = data;
	USART_TxHead = tmphead;
	UCSR0B |= (1<<UDRIE0);
}

ISR(INT0_vect){
	USART_0_write(PD2_get_level());
	needclear=true;
	EIMSK &= (0<<INT0);
}
*/
