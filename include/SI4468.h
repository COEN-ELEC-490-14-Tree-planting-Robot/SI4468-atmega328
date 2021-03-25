/*
 * SI4468.h
 *
 * Created: 2021-03-24 9:17:56 AM
 *  Author: Ling Zhi Mo
 */ 

#include "radio_config_Si4468.h"

//Boot_commands
#define POWER_UP 0x02

//COMMON_COMMANDS
#define NOP 0x00
#define PART_INFO 0x01
#define FUNC_INFO 0x10
#define SET_PROPERTY 0x11
#define GET_PROPERTY 0x12
#define GPIO_PIN_CFG 0x13
#define FIFO_INFO 0x15
#define IRCAL 0x17
#define GET_INT_STATUS 0x20
#define GET_PH_STATUS 0x21
#define GET_CHIP_STATUS 0x23
#define START_TX 0x31
#define START_RX 0x32
#define REQUEST_DEVICE_STATE 0x33
#define CHANGE_STATE 0x34
#define OFFLINE_RECAL 0x38
#define READ_CMD_BUFF 0x44
#define FRR_A_READ 0x50
#define FRR_B_READ 0x51
#define FRR_C_READ 0x53
#define FRR_D_READ 0x57
#define WRITE_TX_FIFO 0x66
#define READ_RX_FIFO 0x77

//Group
#define PROP_GROUP_PA 0x22
#define PROP_GROUP_PKT 0x12 ///< Property group packet config
#define PKT_PROP(prop) ((PROP_GROUP_PKT<<8) | prop)
#define PA_PROP(prop) ((PROP_GROUP_PA<<8) | prop)
#define	PA_PWR_LVL PA_PROP(0x01)
#define PKT_FIELD_2_LENGTH_LOW PKT_PROP(0x12)

#define MAX_PACKET_LEN	128

static const uint8_t config[]  = RADIO_CONFIGURATION_DATA_ARRAY;

#define SI4468_FIXED_LENGTH 0
#define IDLE_STATE SI446X_STATE_READY

typedef enum
{
	SI446X_STATE_NOCHANGE	= 0x00,
	SI446X_STATE_SLEEP		= 0x01, ///< This will never be returned since SPI activity will wake the radio into ::SI446X_STATE_SPI_ACTIVE
	SI446X_STATE_SPI_ACTIVE	= 0x02,
	SI446X_STATE_READY		= 0x03,
	SI446X_STATE_READY2		= 0x04, ///< Will return as ::SI446X_STATE_READY
	SI446X_STATE_TX_TUNE	= 0x05, ///< Will return as ::SI446X_STATE_TX
	SI446X_STATE_RX_TUNE	= 0x06, ///< Will return as ::SI446X_STATE_RX
	SI446X_STATE_TX			= 0x07,
	SI446X_STATE_RX			= 0x08
} si446x_state_t;

#ifndef SI4468_H_
#define SI4468_H_

uint8_t SI4468_Clear_All_Interrupt(void* buff);
uint8_t SI4468_Clear_Some_Interrupts(void* buff, uint8_t clearPH, uint8_t clearMODEM, uint8_t clearCHIP);
uint8_t SI4468_INIT();
uint8_t SI4468_SetState(si446x_state_t newState);
uint8_t SI4468_DoAPI(void* data, uint8_t len, void* out, uint8_t outLen);
uint8_t waitForResponse(void* out, uint8_t outLen, uint8_t useTimeout);
uint8_t getResponse(void* buff, uint8_t len);
si446x_state_t SI4468_GetState(void);
uint8_t SI4468_Sleep();
uint8_t setProperties(uint16_t prop, void* values, uint8_t len);
inline uint8_t setProperty(uint16_t prop, uint8_t value){return setProperties(prop, &value, 1);}
// Read a single property
uint8_t getProperties(uint16_t prop, void* values, uint8_t len);
inline uint8_t getProperty(uint16_t prop) { uint8_t val; getProperties(prop, &val, 1); return val;}
uint8_t SI4468_TX(void* packet, uint8_t len, uint8_t channel, si446x_state_t onTxFinish);
uint8_t SI4468_RX(uint8_t channel);
void SI4468_Read(void* buff, uint8_t len);

//Deprecated functions,Need change
//######################################################################################################################
uint8_t SI4468_WaitCTS();
uint8_t SI4468_NOP();
void SI4468_PART_INFO();
void SI4468_FUNC_INFO();
uint8_t SI4468_GET_CHIP_STATUS();
void SI4468_GET_PH_STATUS();
void SI4468_GET_PROPERTY(uint8_t group, uint8_t numprops, uint8_t startprops);
uint8_t SI4468_SET_PROPERTY(uint8_t group, uint8_t numprops, uint8_t startprops,const uint8_t * params, uint8_t size);
void SI4468_START_TX(uint8_t channel, uint8_t TX_COMPLETE_STATE, bool RETRANSMIT,bool START);
void SI4468_START_RX(uint8_t channel, bool START, uint8_t RXTIMEOUT_STATE, uint8_t RXVALID_STATE, uint8_t RXINVALID_STATE);
uint8_t SI4468_WriteTxDataBuffer(uint8_t * data , uint8_t size);
uint8_t SI4468_ReadRxDataBuffer();
uint8_t SI4468_GPIO_PIN_CFG();
uint8_t SI4468_GET_INT_STATUS(bool);
uint8_t SI4468_REQUEST_DEVICE_STATE();

#endif /* SI4468_H_ */