/*
 * SI4468.c
 *
 * Created: 2021-03-24 9:18:10 AM
 *  Author: mlz96
 */ 
#include <spi_basic.h>
#include "SI4468.h"
#include <atmel_start_pins.h>
#include <util/delay.h>



//Working methods
uint8_t SI4468_WaitCTS(){
	uint8_t bCtsValue;
	int bErrCnt;
	bCtsValue = 0;
	bErrCnt = 0;
	while (bCtsValue!=0xFF) // Wait until radio IC is ready with the data
	{
		PB2_set_level(0); // select radio IC by pulling its nSEL pin low
		_delay_us(10);
		bCtsValue=READ_CMD_BUFF;
		bCtsValue=SPI_0_exchange_byte(bCtsValue); // Read command buffer; send command byte
		PB2_set_level(1); // If CTS is not 0xFF, put NSS high and stay in waiting
		if (++bErrCnt > MAX_CTS)
		{
			return ERROR; // Error handling; if wrong CTS reads exceeds a limit
		}
	}
	return SUCCESS;
}

uint8_t SI4468_DoAPI(void* data, uint8_t len, void* out, uint8_t outLen){
	//uint16_t timeout = 40000;
	//while( SI4468_WAITCTS()==ERROR && !--timeout )
	//_delay_us(10);
	if( SI4468_WaitCTS() == SUCCESS){
		//SPI_0_write_block((uint8_t*)data,len);
		if( ((uint8_t*)data)[0] == IRCAL && waitForResponse(NULL,0,0) == SUCCESS )
		return SUCCESS;
		else if(out != NULL && waitForResponse(out,outLen,1) == SUCCESS )
		return SUCCESS;
		else if( SI4468_WaitCTS() )
		return SUCCESS;
		return ERROR;
	}
	else return ERROR;
}

uint8_t waitForResponse(void* out, uint8_t outLen, uint8_t useTimeout)
{
	// With F_CPU at 8MHz and SPI at 4MHz each check takes about 7us + 10us delay
	uint16_t timeout = 40000;
	while(!getResponse(out, outLen))
	{
		_delay_us(10);
		if(useTimeout && !--timeout)
		{
			//SI446X_CB_CMDTIMEOUT();
			//USART_0_write_block("CMD TIMEOUT",sizeof("CMD TIMEOUT"));
			return ERROR;
		}
	}
	return SUCCESS;
}

uint8_t getResponse(void* buff, uint8_t len)
{
	uint8_t cts = 0xFF;
	// Send command
	SPI_0_write_block(READ_CMD_BUFF,1);
	// Get CTS value
	cts = SPI_0_exchange_byte(cts);
	if(cts){
		// Get response data
		/*
		for(uint8_t i = 0; i<len; i++){
			( (uint8_t*)buff )[i] = 0xFF;
			( (uint8_t*)buff )[i] = SPI_0_exchange_byte( ((uint8_t*)buff)[i] );
			USART_0_write(( (uint8_t*)buff )[i]);
		}
		*/
		SPI_0_exchange_block(buff,len);
	}
	return cts;
}

// Read a fast response register
uint8_t getFRR(uint8_t reg)
{
	uint8_t frr = 0;
	//SPI_0_write_block(reg,1);
	frr = SPI_0_exchange_byte(reg);
	USART_0_write('a');
	USART_0_write(frr);
	return frr;
}

uint8_t SI4468_Clear_All_Interrupt(void* buff)
{
	uint8_t data = GET_INT_STATUS;
	//USART_0_write(0x02);
	//USART_0_write(sizeof(buff));
	uint8_t result = SI4468_DoAPI(&data, sizeof(data), buff, 9);
	
	//USART_0_write_block(buff,sizeof(buff));
	return result;
}

uint8_t SI4468_Clear_Some_Interrupts(void* buff, uint8_t clearPH, uint8_t clearMODEM, uint8_t clearCHIP)
{
	uint8_t data[] = {
		GET_INT_STATUS,
		clearPH,
		clearMODEM,
		clearCHIP
	};
	return SI4468_DoAPI(data, sizeof(data), buff, 8);
}

uint8_t SI4468_INIT(){
	//PD5_set_level(true);
	//_delay_ms(10);
	//USART_0_write(PD5_get_level());
	
	//PD5_set_level(false);
	//_delay_ms(6);
	
	uint8_t buff[17];
	for(uint16_t i = 0, temp = 0; i < sizeof(config)-1; ++i){
		memcpy(buff, &config[i], sizeof(buff));
		temp = buff[0];
		//USART_0_write(SI4468_DoAPI(&buff[1], buff[0], NULL, 0));
		i += temp;
	}
	
	SI4468_Clear_All_Interrupt(NULL);
}

uint8_t SI4468_SetState(si446x_state_t newState)
{
	uint8_t data[] = {
		CHANGE_STATE,
		newState
	};
	SI4468_DoAPI(data, sizeof(data), NULL, 0);
}

si446x_state_t SI4468_GetState(void)
{
	
	uint8_t state = getFRR(FRR_C_READ);
	/*
	if(state == SI446X_STATE_TX_TUNE)
	state = SI446X_STATE_TX;
	else if(state == SI446X_STATE_RX_TUNE)
	state = SI446X_STATE_RX;
	else if(state == SI446X_STATE_READY2)
	state = SI446X_STATE_READY;
	*/
	USART_0_write(state);
	return (si446x_state_t)state;
}

uint8_t SI4468_Sleep()
{
	if(SI4468_GetState() == SI446X_STATE_TX)
	return ERROR;
	SI4468_SetState(SI446X_STATE_SLEEP);
	return SUCCESS;
}

uint8_t setProperties(uint16_t prop, void* values, uint8_t len){
	// len must not be greater than 12
	uint8_t data[16] = {
		SET_PROPERTY,
		(uint8_t)(prop>>8),
		len,
		(uint8_t)prop
	};
	// Copy values into data, starting at index 4
	memcpy(data + 4, values, len);
	return SI4468_DoAPI(data, len + 4, NULL, 0);
}

// Read a bunch of properties
uint8_t getProperties(uint16_t prop, void* values, uint8_t len)
{
	uint8_t data[] = {
		GET_PROPERTY,
		(uint8_t)(prop>>8),
		len,
		(uint8_t)prop
	};
	return SI4468_DoAPI(data, sizeof(data), values, len);
}

void SI4468_SetTxPower(uint8_t pwr)
{
	setProperty(PA_PWR_LVL, pwr);
}

uint8_t SI4468_TX(void* packet, uint8_t len, uint8_t channel, si446x_state_t onTxFinish)
{
	// TODO what happens if len is 0?
	#if SI4468_FIXED_LENGTH
	// Stop the unused parameter warning
	((void)(len));
	#endif
		if(SI4468_GetState() == SI446X_STATE_TX) // Already transmitting
			return ERROR;

		// TODO collision avoid or maybe just do collision detect (RSSI jump)

		SI4468_SetState(IDLE_STATE);
		//clearFIFO();
		SI4468_Clear_Some_Interrupts(NULL, 0, 0, 0xFF);

			// Load data to FIFO
		SPI_0_write_block(WRITE_TX_FIFO,1);
		#if !SI4468_FIXED_LENGTH
		SPI_0_write_block(len,1);
		//for(uint8_t i=0;i<len;i++)
		SPI_0_write_block(((uint8_t*)packet),len);
		#else
		SPI_0_write_block(((uint8_t*)packet),SI4468_FIXED_LENGTH,);
		#endif

		#if !SI446X_FIXED_LENGTH
		// Set packet length
		setProperty(PKT_FIELD_2_LENGTH_LOW, len);
		#endif

		// Begin transmit
		uint8_t data[] = {
			START_TX,
			channel,
			(uint8_t)(onTxFinish<<4),
			0,
			SI4468_FIXED_LENGTH,
			0,
			0
		};
		SI4468_DoAPI(data, sizeof(data), NULL, 0);

		#if !SI4468_FIXED_LENGTH
		// Reset packet length back to max for receive mode
		setProperty(PKT_FIELD_2_LENGTH_LOW, MAX_PACKET_LEN);
		#endif
		
	return SUCCESS;
}

uint8_t SI4468_RX(uint8_t channel)
{
		SI4468_SetState(IDLE_STATE);
		//clearFIFO();
		//fix_invalidSync_irq(0);
		//Si446x_setupCallback(SI446X_CBS_INVALIDSYNC, 0);
		//setProperty(SI446X_PKT_FIELD_2_LENGTH_LOW, MAX_PACKET_LEN); // TODO ?
		SI4468_Clear_Some_Interrupts(NULL, 0, 0, 0xFF); // TODO needed?

		// TODO RX timeout to sleep if WUT LDC enabled
		uint8_t data[] = {
			START_RX,
			channel,
			0,
			0,
			SI4468_FIXED_LENGTH,
			SI446X_STATE_NOCHANGE, // RX Timeout
			IDLE_STATE, // RX Valid
			SI446X_STATE_SLEEP // IDLE_STATE // RX Invalid (using SI446X_STATE_SLEEP for the INVALID_SYNC fix)
		};
		return SI4468_DoAPI(data, sizeof(data), NULL, 0);
}

void SI4468_Read(void* buff, uint8_t len){
	SPI_0_write_block(READ_RX_FIFO,1);
	for(uint8_t i=0;i<len;i++)
		((uint8_t*)buff)[i] = SPI_0_exchange_byte(0xFF);
}

//Deprecated methods, need to replace every method below.
//#################################################################
uint8_t SI4468_POWER_UP(){
	PB2_set_level(false);
	_delay_us(10);
	uint8_t data[] = {\
		POWER_UP,\
		0x01,\
		0x00,\
		0x01,\
		0xC9,\
		0xC3,\
		0x80
	};
	SPI_0_write_block(data,7);
	/*
	SPI_0_write_block(POWER_UP,sizeof(POWER_UP));
	SPI_0_write_block(0x01,sizeof(uint8_t));
	SPI_0_write_block(0x00,sizeof(uint8_t));
	SPI_0_write_block(0x01,sizeof(uint8_t));
	SPI_0_write_block(0xC9,sizeof(uint8_t));
	SPI_0_write_block(0xC3,sizeof(uint8_t));
	SPI_0_write_block(0x80,sizeof(uint8_t));
	*/
	if(SI4468_WaitCTS()==SUCCESS){
		PB2_set_level(true);
		return SUCCESS;
	}
	PB2_set_level(true);
	return ERROR;
	
}

uint8_t SI4468_GetResponse(uint8_t *pbRespData,uint8_t bRespLength) //Get response from the chip (used after a command)
{
	uint8_t bCtsValue;
	int bErrCnt;
	bCtsValue = 0;
	bErrCnt = 0;
	while (bCtsValue!=0xFF) // Wait until radio IC is ready with the data
	{
		PB2_set_level(0); // select radio IC by pulling its nSEL pin low
		SPI_0_write_block(READ_CMD_BUFF,sizeof(READ_CMD_BUFF)); // Read command buffer; send command byte
		SPI_0_read_block(bCtsValue,sizeof(bCtsValue));
		if(bCtsValue != 0xFF)
		{
			PB2_set_level(1);
		}
	}
	if(bErrCnt++ > MAX_CTS)
	{
		return 1;
	}
	SPI_0_read_block(pbRespData,bRespLength); // CTS value ok, get the response data from the radioIC
	PB2_set_level(1); // de-select radio IC by putting its nSEL pin high

	return 0;
} 

uint8_t SI4468_NOP(){
	//SI4468_set_nSEL(false);
	SPI_0_write_block(NOP,sizeof(NOP));
	uint8_t response;

	while(!PD4_get_level()){
	}
	SPI_0_read_block(response,sizeof(uint8_t));
	USART_0_write(response);
	
	//SI4468_set_nSEL(true);
	if(!PD4_get_level())
		return 0;
	else
		return 1;
	
}

void SI4468_PART_INFO(){
	SPI_0_write_block(PART_INFO,sizeof(PART_INFO));
	char * a;
	SPI_0_read_block(a,sizeof(uint8_t)*9);
	//USART_0_write_block(a,sizeof(uint8_t)*9);
	//USART_0_write('\n');
}

void SI4468_FUNC_INFO(){
	SPI_0_write_block(FUNC_INFO,sizeof(FUNC_INFO));
	char * a;
	uint8_t temp;
	SPI_0_read_block(a,sizeof(uint8_t)*7);
	//USART_0_write_block(a,sizeof(uint8_t)*7);
	//USART_0_write('\n');
}

uint8_t SI4468_GET_CHIP_STATUS(){
	PB2_set_level(false);
	SPI_0_write_block(GET_CHIP_STATUS,sizeof(GET_CHIP_STATUS));
	//clearing interrupt
	SPI_0_write_block(0x00,sizeof(uint8_t)); 
	SPI_0_write_block(0x00,sizeof(uint8_t));
	SPI_0_write_block(0x00,sizeof(uint8_t));
	uint8_t * databuffer;
	if(SI4468_WaitCTS()==SUCCESS || PD4_get_level()){
		SPI_0_read_block(databuffer,3);//clear read buffer
		PB2_set_level(true);
		//USART_0_write_block(databuffer,3);
		free(databuffer);
		databuffer=NULL;
		return SUCCESS;
	}
	databuffer=NULL;
	PB2_set_level(true);
	return ERROR;
}

void SI4468_GET_PH_STATUS(){
	SI4468_set_nSEL(false);
	
	SPI_0_write_block(GET_PH_STATUS,sizeof(GET_PH_STATUS));
	
	char * a;
	while(!PD4_get_level()){	
	}
	SPI_0_read_block(a,sizeof(uint8_t)*10);
	//USART_0_write_block(a,sizeof(uint8_t)*10);
	SI4468_set_nSEL(true);
	free(a);
	a=NULL;
}

void SI4468_START_TX(uint8_t channel, uint8_t TX_COMPLETE_STATE, bool RETRANSMIT,bool START){
	
	PB2_set_level(false);
	SPI_0_write_block(START_TX,sizeof(START_TX));
	SPI_0_write_block(channel,sizeof(channel));
	uint8_t param2 = (TX_COMPLETE_STATE<<4) | (0<<3) | (RETRANSMIT<<2) |(0<<1)|(!START<<0);
	SPI_0_write_block(param2,sizeof(uint8_t));
	SPI_0_write_block(0,sizeof(uint8_t));
	SPI_0_write_block(0,sizeof(uint8_t));
	if(SI4468_WaitCTS())
		USART_0_write(SUCCESS);
	PB2_set_level(true);
}

void SI4468_START_RX(uint8_t channel, bool START, uint8_t RXTIMEOUT_STATE, uint8_t RXVALID_STATE, uint8_t RXINVALID_STATE){
	PB2_set_level(false);
	SPI_0_write_block(START_RX,sizeof(START_RX));
	SPI_0_write_block(channel,sizeof(channel));
	SPI_0_write_block((0|(!START<<0)),sizeof(uint8_t));
	SPI_0_write_block(0x00,sizeof(uint8_t));
	SPI_0_write_block(0x00,sizeof(uint8_t));
	SPI_0_write_block(0x0F&RXTIMEOUT_STATE,sizeof(uint8_t));
	SPI_0_write_block(0x0F&RXVALID_STATE,sizeof(uint8_t));
	SPI_0_write_block(0x0F&RXINVALID_STATE,sizeof(uint8_t));
	
	//if(SI4468_WAITCTS()==SUCCESS);
		//USART_0_write(0b00001111);
	PB2_set_level(true);
}

uint8_t SI4468_ReadRxDataBuffer()
{
	PB2_set_level(0); // select radio IC by pulling its nSEL pin low
	SPI_0_write_block(READ_RX_FIFO,sizeof(READ_RX_FIFO)); // Send Rx read command
	uint8_t temp=0;
	SPI_0_read_block(temp, sizeof(uint8_t));// Write Tx FIFO
	if(temp!=0x00)
		USART_0_write(temp);
	PB2_set_level(1);
	return SUCCESS;
}

uint8_t SI4468_WriteTxDataBuffer(uint8_t * data , uint8_t size) // Write Tx FIFO
{
	PB2_set_level(0); // select radio IC by pulling its nSEL pin low
	SPI_0_write_block(WRITE_TX_FIFO,sizeof(WRITE_TX_FIFO)); // Send Rx read command
	SPI_0_write_block(data,size);
	if(SI4468_WaitCTS()==SUCCESS || PD4_get_level()){
		PB2_set_level(true);
		return SUCCESS;
	}
	PB2_set_level(1);
}


uint8_t SI4468_REQUEST_DEVICE_STATE(){
	PB2_set_level(0);
	SPI_0_write_block(REQUEST_DEVICE_STATE,sizeof(REQUEST_DEVICE_STATE));
	uint8_t *data = (uint8_t*)malloc(2);
	if(SI4468_WaitCTS()==SUCCESS){
		SPI_0_read_block(data,2);
		PB2_set_level(1);
		free(data);
		return SUCCESS;
	}
	PB2_set_level(1);
	return ERROR;
}