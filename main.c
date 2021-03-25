#include <atmel_start.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <string.h>

#define ERROR 0x46
#define SUCCESS 0x50
#define PACKET_NONE		0
#define PACKET_OK		1
#define PACKET_INVALID	2
#define CHANNEL 20
#define MAX_PACKET_SIZE 10
typedef struct{
	uint8_t ready;
	int16_t rssi;
	uint8_t length;
	uint8_t buffer[MAX_PACKET_SIZE];
} pingInfo_t;

#define USART_RX_BUFFER_SIZE 64     /* 2,4,8,16,32,64,128 or 256 bytes */
#define USART_TX_BUFFER_SIZE 64     /* 2,4,8,16,32,64,128 or 256 bytes */
#define USART_RX_BUFFER_MASK (USART_RX_BUFFER_SIZE - 1)
#define USART_TX_BUFFER_MASK (USART_TX_BUFFER_SIZE - 1)

#if (USART_RX_BUFFER_SIZE & USART_RX_BUFFER_MASK)
#error RX buffer size is not a power of 2
#endif
#if (USART_TX_BUFFER_SIZE & USART_TX_BUFFER_MASK)
#error TX buffer size is not a power of 2
#endif

/* Static Variables */
static unsigned char USART_RxBuf[USART_RX_BUFFER_SIZE];
static volatile unsigned char USART_RxHead;
static volatile unsigned char USART_RxTail;
static unsigned char USART_TxBuf[USART_TX_BUFFER_SIZE];
static volatile unsigned char USART_TxHead;
static volatile unsigned char USART_TxTail;

/* Prototypes */
void USART1_Init(unsigned int ubrr_val);
unsigned char USART1_Receive(void);
void USART1_Transmit(unsigned char data);
static uint8_t Interrupt_vector[9];
static uint8_t cliflag;
static uint8_t debugging=0;
int main_station(void)
{

	//USART_0_write(SI4468_GetState());
	USART_0_write(SI4468_RX(CHANNEL));
	//USART_0_write(SI4468_GetState());
	
	while (1)
	{
		//USART_0_write(PD6_get_level());
		if (PD4_get_level())
			USART_0_write_block("CTS clear\n",sizeof("CTS clear\n"));
		//if(PD2_get_level()==0)
			//USART_0_write(SI4468_Clear_All_Interrupt(temp));
		//USART_0_write(PD2_get_level());
		//if(SI4468_GetState()==SI446X_STATE_SLEEP)
		//USART_0_write_block(temp,8);
		_delay_ms(3000);
	}
}

int main_turtlebot(void)
{
	//USART_0_write(SI4468_TX(1));
	while (1)
	{
		if (PD4_get_level())
			//USART_0_write_block("CTS clear\n",sizeof("CTS clear\n"));
			USART_0_write(SI4468_TX("Hello World",sizeof("Hello World"),CHANNEL,SI446X_STATE_TX));
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
	
	main_station();
	//main_turtlebot();
	
	
	/*
	while (1)
	{
		//USART_0_write(PD4_get_level());
		USART_0_write(SI4468_GetState());
		//USART_0_write(SI4468_WAITCTS());
		_delay_ms(3000);
	}
	*/
	return 0;
}

ISR(USART_RX_vect)
{
	unsigned char data;
	unsigned char tmphead;

	/* Read the received data */
	data = UDR0;
	
	//Process data
	USART_0_write(data);
	
	tmphead = (USART_RxHead + 1) & USART_RX_BUFFER_MASK;
	USART_RxHead = tmphead;
	
	if (tmphead == USART_RxTail) {
	}
	USART_RxBuf[tmphead] = data;
	
}

ISR(USART_UDRE_vect)
{
	unsigned char tmptail;

	/* Check if all data is transmitted */
	if (USART_TxHead != USART_TxTail) {
		/* Calculate buffer index */
		tmptail = (USART_TxTail + 1) & USART_TX_BUFFER_MASK;
		/* Store new index */
		USART_TxTail = tmptail;
		/* Start transmission */
		UDR0 = USART_TxBuf[tmptail];
		} else {
		/* Disable UDRE interrupt */
		UCSR0B &= ~(1<<UDRIE0);
	}
	
	//USART_0_write('T');
}

unsigned char USART1_Receive(void)
{
	unsigned char tmptail;
	
	/* Wait for incoming data */
	while (USART_RxHead == USART_RxTail);
	/* Calculate buffer index */
	tmptail = (USART_RxTail + 1) & USART_RX_BUFFER_MASK;
	/* Store new index */
	USART_RxTail = tmptail;
	/* Return data */
	return USART_RxBuf[tmptail];
}

void USART1_Transmit(unsigned char data)
{
	unsigned char tmphead;
	
	/* Calculate buffer index */
	tmphead = (USART_TxHead + 1) & USART_TX_BUFFER_MASK;
	/* Wait for free space in buffer */
	while (tmphead == USART_TxTail);
	/* Store data in buffer */
	USART_TxBuf[tmphead] = data;
	/* Store new index */
	USART_TxHead = tmphead;
	/* Enable UDRE interrupt */
	UCSR0B |= (1<<UDRIE0);
}

ISR(INT0_vect){
	EIMSK = (0<<INT0);
	USART_0_write(PD2_get_level());
	SI4468_Clear_All_Interrupt(Interrupt_vector);
	for(uint8_t i = 0;i<9;++i)
		USART_0_write(Interrupt_vector[i]);
	USART_0_write(PD2_get_level());
	EIMSK = (1<<INT0);
	//SI4468_GET_INT_STATUS(false);
	/*
	uint8_t buff[8];
	uint8_t temp[8];
	SI4468_DoAPI(GET_INT_STATUS,sizeof(GET_INT_STATUS),buff,8);
	USART_0_write_block(temp,8);
	*/
}
