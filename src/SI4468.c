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
uint8_t SI4468_DOAPI(void* data, uint8_t len, void* out, uint8_t outLen){
	//uint16_t timeout = 40000;
	//while( SI4468_WAITCTS()==ERROR && !--timeout )
	//_delay_us(10);
	if( SI4468_WAITCTS() == SUCCESS){
		SPI_0_write_block((uint8_t*)data,len);
		if( ((uint8_t*)data)[0] == IRCAL && waitForResponse(NULL,0,0) == SUCCESS )
		return SUCCESS;
		else if(out != NULL && waitForResponse(out,outLen,1) == SUCCESS )
		return SUCCESS;
		else if( SI4468_WAITCTS() )
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
		for(uint8_t i = 0; i<len; i++){
			( (uint8_t*)buff )[i] = 0xFF;
			( (uint8_t*)buff )[i] = SPI_0_exchange_byte( ((uint8_t*)buff)[i] );
		}
	}
	return cts;
}

uint8_t SI4468_CLEAR_INTERRUPT(void* buff)
{
	uint8_t data = GET_INT_STATUS;
	SI4468_DOAPI(&data, sizeof(data), buff, 8);
}

uint8_t SI4468_INIT(){
	PD5_set_level(true);
	_delay_ms(10);
	//USART_0_write(PD5_get_level());
	
	PD5_set_level(false);
	_delay_ms(6);
	
	uint8_t buff[17];
	for(uint16_t i = 0, temp = 0; i < sizeof(config)-1; ++i)
	{
		memcpy(buff, &config[i], sizeof(buff));
		temp = buff[0];
		SI4468_DOAPI(&buff[1], buff[0], NULL, 0);
		i += temp;
	}
	
	SI4468_CLEAR_INTERRUPT(NULL);
}

uint8_t SI4468_SETSTATE(si446x_state_t newState)
{
	uint8_t data[] = {
		CHANGE_STATE,
		newState
	};
	SI4468_DOAPI(data, sizeof(data), NULL, 0);
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
	if(SI4468_WAITCTS()==SUCCESS){
		PB2_set_level(true);
		return SUCCESS;
	}
	PB2_set_level(true);
	return ERROR;
	
}

uint8_t SI4468_WAITCTS(){
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

uint8_t SI4468_GPIO_PIN_CFG(){
	PB2_set_level(false);
	SPI_0_write_block(GPIO_PIN_CFG,sizeof(GPIO_PIN_CFG));
	SPI_0_write_block(0x13,sizeof(uint8_t));
	SPI_0_write_block(0x08,sizeof(uint8_t));
	SPI_0_write_block(0x20,sizeof(uint8_t));
	SPI_0_write_block(0x21,sizeof(uint8_t));
	SPI_0_write_block(0x00,sizeof(uint8_t));
	SPI_0_write_block(0x00,sizeof(uint8_t));
	SPI_0_write_block(0x00,sizeof(uint8_t));
	if(SI4468_WAITCTS()==SUCCESS || PD4_get_level()){
		PB2_set_level(true);
		return SUCCESS;
	}
	PB2_set_level(true);
	return ERROR;
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
	if(SI4468_WAITCTS()==SUCCESS || PD4_get_level()){
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

void SI4468_GET_PROPERTY(uint8_t group, uint8_t numprops, uint8_t startprops){
	SPI_0_write_block(GET_PROPERTY,sizeof(GET_PROPERTY));
	SPI_0_write_block(group,sizeof(group));
	SPI_0_write_block(numprops,sizeof(numprops));
	SPI_0_write_block(startprops,sizeof(startprops));
	char * a;
	SPI_0_read_block(a,sizeof(uint8_t)*(numprops+1));
	//USART_0_write_block(a,sizeof(uint8_t)*(numprops+1));
	//USART_0_write('\n');
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
	if(SI4468_WAITCTS())
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
	if(SI4468_WAITCTS()==SUCCESS || PD4_get_level()){
		PB2_set_level(true);
		return SUCCESS;
	}
	PB2_set_level(1);
}

uint8_t SI4468_SET_PROPERTY(uint8_t group, uint8_t numprops, uint8_t startprops,const uint8_t * params, uint8_t size){
	
	PB2_set_level(0); // select radio IC by pulling its nSEL pin low
	SPI_0_write_block(SET_PROPERTY,sizeof(SET_PROPERTY)); // Send Rx read command
	SPI_0_write_block(group,sizeof(group));
	SPI_0_write_block(numprops,sizeof(numprops));
	SPI_0_write_block(startprops,sizeof(startprops));
	for(uint8_t i = 0;i<size;i++)
		SPI_0_write_block(params[i],sizeof(uint8_t));
	if(SI4468_WAITCTS()==SUCCESS){
		PB2_set_level(true);
		return SUCCESS;
	}
	PB2_set_level(true);
	return ERROR;
}

uint8_t SI4468_CHANGE_STATE(uint8_t state){
	PB2_set_level(0); // select radio IC by pulling its nSEL pin low
	SPI_0_write_block(CHANGE_STATE,sizeof(CHANGE_STATE)); // Send Rx read command
	SPI_0_write_block(state&0x0F,sizeof(state));
	if(SI4468_WAITCTS()==SUCCESS || PD4_get_level()){
		PB2_set_level(true);
		return SUCCESS;
	}
	PB2_set_level(true);
	return ERROR;
}

uint8_t SI4468_GET_INT_STATUS(bool debug){
	PB2_set_level(false);
	SPI_0_write_block(GET_INT_STATUS,sizeof(GET_INT_STATUS));
	//clearing interrupt
	SPI_0_write_block(0x00,sizeof(uint8_t));
	SPI_0_write_block(0x00,sizeof(uint8_t));
	SPI_0_write_block(0x00,sizeof(uint8_t));
	uint8_t * databuffer;
	uint8_t TX_STATES,RX_STATES;
	if(SI4468_WAITCTS()==SUCCESS){
		SPI_0_read_block(databuffer,8);//clear read buffer
		PB2_set_level(true);
		
		if(debug){
			//USART_0_write_block(databuffer,8);
			TX_STATES=(databuffer+4);
			RX_STATES=(databuffer+6);
			USART_0_write(0x00);
			USART_0_write(TX_STATES);
			USART_0_write(0xFF);
			USART_0_write(RX_STATES);
		}
		free(databuffer);
		databuffer=NULL;
		return SUCCESS;
	}
	databuffer=NULL;
	PB2_set_level(true);
	return ERROR;
}

uint8_t SI4468_REQUEST_DEVICE_STATE(){
	PB2_set_level(0);
	SPI_0_write_block(REQUEST_DEVICE_STATE,sizeof(REQUEST_DEVICE_STATE));
	uint8_t *data = (uint8_t*)malloc(2);
	if(SI4468_WAITCTS()==SUCCESS){
		SPI_0_read_block(data,2);
		PB2_set_level(1);
		free(data);
		return SUCCESS;
	}
	PB2_set_level(1);
	return ERROR;
}