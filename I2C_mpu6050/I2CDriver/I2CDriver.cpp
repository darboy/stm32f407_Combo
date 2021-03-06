#include "I2CDriver.h"
#include "SEGGER_RTT.h"
#include "string.h"

static int32_t print_count = 0;

uint32_t I2CDriver::WAIT_FOR_FLAG(uint32_t flag, FlagStatus value, int timeout, int errorcode)	
{
//like a interrupt,to do this function is to make the "interrupt" occur certainly.
//also add a counter to avoid the 
	int I2CTimeout = timeout;
	while(I2CTimeout && (I2C_GetFlagStatus(I2C1, flag) != value))
	{
		I2CTimeout--;
		if(I2CTimeout < 0 ){I2CTimeout = 0;}
		if(I2CTimeout == 0)
		{
			I2Cx_TIMEOUT_UserCallback(errorcode);
			return 1;
		}
	}
}

uint32_t I2CDriver::WAIT_FOR_EVENT(uint32_t event, int timeout, int errorcode)
{
	int I2CTimeout = timeout;	
	while(I2CTimeout && (!I2C_CheckEvent(I2C1,event)))			//attention the judge order
	{
		I2CTimeout--;
		if(I2CTimeout < 0 ){I2CTimeout = 0;}
		if(I2CTimeout == 0)
		{
			I2Cx_TIMEOUT_UserCallback(errorcode);
			return 1;
		}
	}	
}
														
uint32_t I2CDriver::I2Cx_TIMEOUT_UserCallback(char value)
{
	SEGGER_RTT_printf(0,"\r\n Failed code:%d \r\n", value);
	return 1;
}

void I2CDriver::releaseBusByForce()
{
	I2C_GenerateSTOP(I2C1, ENABLE);
	I2C_SoftwareResetCmd(I2C1, ENABLE);
	I2C_SoftwareResetCmd(I2C1, DISABLE);
	I2C_DeInit(I2C1);
	_i2c.I2C_ClockSpeed = _clock_speed;
    _i2c.I2C_Mode = _i2c_mode;
    _i2c.I2C_DutyCycle = _duty_cycle;
    _i2c.I2C_OwnAddress1 = _own_address;
    _i2c.I2C_Ack = I2C_Ack_Enable;
    _i2c.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_Cmd(I2C1, ENABLE);
	I2C_Init(I2C1, &_i2c);
}

void I2CDriver::I2C_Mode_Config(void)
{
	I2C_InitTypeDef I2C_InitStructure;
	I2C_InitStructure.I2C_Mode = I2C_Mode_I2C ;
	I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
	I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_InitStructure.I2C_ClockSpeed = 40000;
	I2C_Init(I2C1, &I2C_InitStructure); 
	I2C_Cmd (I2C1,ENABLE);
	I2C_AcknowledgeConfig(I2C1, ENABLE); 
}

void I2CDriver::I2C_GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1,ENABLE); 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
}

void I2CDriver::initialize()
{
	//it is refer to the internet ,can solove some problem.
/************************************************************/
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG, ENABLE);
	_gpio.GPIO_Pin = _scl_gpio_pin | _sda_gpio_pin;     
	_gpio.GPIO_Mode = GPIO_Mode_OUT;
    _gpio.GPIO_OType = GPIO_OType_PP;  
    _gpio.GPIO_Speed = GPIO_Speed_50MHz;        
	GPIO_Init(_gpio_chanel, &_gpio);
    GPIO_SetBits(_gpio_chanel,_sda_gpio_pin);
    GPIO_SetBits(_gpio_chanel,_scl_gpio_pin);
/************************************************************/
//config the i2c gpio ,conect to the AF of i2c,the speed of gpio be set low is proper.
/************************************************************/
	_gpio.GPIO_Mode = GPIO_Mode_AF;
	_gpio.GPIO_OType = GPIO_OType_OD;
	_gpio.GPIO_Pin = _scl_gpio_pin | _sda_gpio_pin;
	_gpio.GPIO_PuPd = GPIO_PuPd_NOPULL;
	_gpio.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_PinAFConfig(_gpio_chanel,GPIO_PinSource8,GPIO_AF_I2C1);
	GPIO_PinAFConfig(_gpio_chanel,GPIO_PinSource7,GPIO_AF_I2C1);
	GPIO_Init(_gpio_chanel, &_gpio);
/*****************************************************************/	
//config the i2c,the speed is set the fast mode,
//if change it ,meanwhile change the param like "TIMEOUT",must measure the count of while function.
/*****************************************************************/	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1,ENABLE);
    _i2c.I2C_ClockSpeed = _clock_speed;
    _i2c.I2C_Mode = _i2c_mode;
    _i2c.I2C_DutyCycle = _duty_cycle;
    _i2c.I2C_OwnAddress1 = _own_address;
    _i2c.I2C_Ack = I2C_Ack_Enable;
    _i2c.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
	I2C_Cmd(I2C1, ENABLE);
	I2C_Init(I2C1, &_i2c); 
/*****************************************************************/
}

int I2CDriver::writeArrary(uint8_t w_addr, const uint8_t* p_data, uint16_t len)
{	
	uint16_t i = 0;
	int send_count = I2Cx_SEND_TIMEOUT;
	__IO uint32_t I2CTimeout = I2Cx_LONG_TIMEOUT;
	//judge if busy or not
	WAIT_FOR_FLAG(I2C_FLAG_BUSY,RESET,I2Cx_LONG_TIMEOUT,1);
	//start by write CR1
	I2C_GenerateSTART(I2C1, ENABLE);
	//judge if SB bit of SR1 is 1,if right ,the send signal is finished.
	WAIT_FOR_FLAG(I2C_FLAG_SB, SET, I2Cx_FLAG_TIMEOUT, 2);
	//"|" with OAR1 ,then write in DR 
	I2C_Send7bitAddress(I2C1,(w_addr /*<< 1*/), I2C_Direction_Transmitter);
	//judge if ADDR bit of SR1 is 1,if right ,the address has been send.
	WAIT_FOR_FLAG(I2C_FLAG_ADDR, SET, I2Cx_FLAG_TIMEOUT, 3);
	//judge if TXE bit of SR1 is 1,if right, the DR is empty. now test for determining if it is need.
	WAIT_FOR_FLAG(I2C_FLAG_TXE, SET, I2Cx_FLAG_TIMEOUT, 4);
	//read to clean (it is a jb move ,fuck)
	CLEAR_ADDR_BIT;
	while ((i < len) && send_count)
	{
		I2C_SendData(I2C1, *p_data);
		WAIT_FOR_FLAG(I2C_FLAG_TXE, SET, I2Cx_FLAG_TIMEOUT, 5);
		i++; p_data++;
		//avoid block here
		/***********************************/
		send_count--;
		if(send_count < 0 ){send_count = 0;}
		/***********************************/
	}
	WAIT_FOR_FLAG(I2C_FLAG_BTF, SET, I2Cx_FLAG_TIMEOUT, 6);
	I2C_GenerateSTOP(I2C1, ENABLE);
	return 0;
}

int I2CDriver::writeByte(uint8_t w_addr, const uint8_t* p_data)
{
	return writeArrary(w_addr, p_data, 1);
}

void I2CDriver::I2C_ByteWrite(uint8_t REG_Address,uint8_t REG_data)
{		
	uint8_t data[2] = {REG_Address, REG_data};
	writeArrary(0xd0, data, 2);
}

int I2CDriver::i2c_write(uint8_t hw_address, uint8_t reg_address, uint8_t len, uint8_t* data)
{
	uint8_t * data_arrary;
	memcpy(data_arrary,&reg_address,1);
	memcpy(data_arrary+1,data,len);
	writeArrary(hw_address << 1, data, len+1);
	return 0;	
}

uint8_t I2CDriver::I2C_ByteRead(uint8_t REG_Address)
{
	uint8_t return_data;
	readByte(0xD0, REG_Address, &return_data, 1);
	return return_data;	
}

int I2CDriver::i2c_read(uint8_t addr, uint8_t reg, uint8_t len, uint8_t *buf)
{
	readByte(addr << 1, reg, buf, len);
	return 0;
}

int I2CDriver::readByte(uint8_t slave_address, uint8_t slave_register_address, uint8_t* p_data, uint16_t len)
{
	int receive_count = I2Cx_RECIEVE_TIMEOUT;
	
	__IO uint32_t I2CTimeout = I2Cx_LONG_TIMEOUT;

	WAIT_FOR_FLAG(I2C_FLAG_BUSY,RESET,I2Cx_LONG_TIMEOUT,7);;

	I2C_GenerateSTART(I2C1,ENABLE);

	WAIT_FOR_EVENT(I2C_EVENT_MASTER_MODE_SELECT, I2Cx_LONG_TIMEOUT, 8);

	I2C_Send7bitAddress(I2C1,slave_address,I2C_Direction_Transmitter);

	WAIT_FOR_EVENT(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED, I2Cx_LONG_TIMEOUT, 9);
	
	I2C_Cmd(I2C1,ENABLE);
	
	I2C_SendData(I2C1,slave_register_address);

	WAIT_FOR_EVENT(I2C_EVENT_MASTER_BYTE_TRANSMITTED, I2Cx_LONG_TIMEOUT, 10);
	
	I2C_GenerateSTART(I2C1,ENABLE);

	WAIT_FOR_EVENT(I2C_EVENT_MASTER_MODE_SELECT, I2Cx_LONG_TIMEOUT, 11);
	
	I2C_Send7bitAddress(I2C1,slave_address,I2C_Direction_Receiver);
	
	WAIT_FOR_EVENT(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED, I2Cx_LONG_TIMEOUT, 12);
	
	while(len && receive_count)
	{
		if(len == 1)
		{
		  I2C_AcknowledgeConfig(I2C1, DISABLE);
		  I2C_GenerateSTOP(I2C1, ENABLE);
		}
		if(I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED))
		{
		  *p_data = I2C_ReceiveData(I2C1);
		  p_data++;
		  len--;
		}
		else
		{
			receive_count--;
			if(receive_count < 0 ){receive_count = 0;}
		}
	}
	CLEAR_ADDR_BIT;
	I2C_AcknowledgeConfig(I2C1, ENABLE);
	return 0;
} 
