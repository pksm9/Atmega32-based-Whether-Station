/*

		TWI MAIN FILE
		twi.c
*/

#include "twi.h"

void twi_init()
{
	DDRC = 0x03;								//--- PORTC Last two bit as Output
	PORTC = 0x03;
	
	usart_init();								//--- Usart Initialization
	usart_msg("CODE-N-LOGIC I2C:");				//--- Send String to Com Port of PC
	usart_tx(0x0d);								//--- Next Line
	
	TWCR &= ~(1<<TWEN);							//--- Diable TWI
	TWBR = BITRATE(TWSR = 0x00);				//--- Bit rate with prescaler 4
	TWCR = (1<<TWEN);							//--- Enable TWI
	_delay_us(10);								//--- Delay
}

/* Function to Send Start Condition */

void twi_start()
{
	TWCR= (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);		//--- Start Condition as per Datasheet
	while(!(TWCR & (1<<TWINT)));				//--- Wait till start condition is transmitted to Slave
	while(TW_STATUS != TW_START);				//--- Check for the acknowledgment 0x08 = TW_START
	usart_msg("Start Exe.");					//--- Feedback msg to check for error
	usart_tx(0x0D);								//--- Next Line
}

/* Function to Send Slave Address for Write operation */

void twi_write_cmd(unsigned char address)
{
	TWDR=address;								//--- SLA Address and write instruction
	TWCR=(1<<TWINT)|(1<<TWEN);					//--- Clear TWI interrupt flag,Enable TWI
	while (!(TWCR & (1<<TWINT)));				//--- Wait till complete TWDR byte transmitted to Slave
	while(TW_STATUS != TW_MT_SLA_ACK);			//--- Check for the acknowledgment
	usart_msg("ACK Received for MT SLA");		//--- Feedback msg to check for error 
	usart_tx(0x0D);								//--- Next Line
}

/* Function to Send Data to Slave Device  */

void twi_write_dwr(unsigned char data)
{
	TWDR=data;									//--- Put data in TWDR
	TWCR=(1<<TWINT)|(1<<TWEN);					//--- Clear TWI interrupt flag,Enable TWI
	while (!(TWCR & (1<<TWINT)));				//--- Wait till complete TWDR byte transmitted to Slave
	while(TW_STATUS != TW_MT_DATA_ACK);			//--- Check for the acknowledgment
	usart_msg("ACK Received for MT Data");		//--- Feedback msg to check error
	usart_tx(0x0D);								//--- Next Line

}

/* Function to Send Stop Condition */

void twi_stop()
{
	TWCR = (1<<TWINT)|(1<<TWEN)|(1<<TWSTO);		//--- Stop Condition as per Datasheet
}

/* Function to Send Repeated Start Condition */


void twi_repeated_start()
{
	TWCR= (1<<TWINT)|(1<<TWSTA)|(1<<TWEN);		//--- Repeated Start Condition as per Datasheet
	while(!(TWCR & (1<<TWINT)));				//--- Wait till restart condition is transmitted to Slave
	while(TW_STATUS != TW_REP_START);			//--- Check for the acknowledgment
	usart_msg("Repeated Start Exe.");			//--- Feedback msg to check error
	usart_tx(0x0D);								//--- Next Line
}


/* Function to Send Read Acknowledgment */

char twi_read_ack()
{
	TWCR=(1<<TWEN)|(1<<TWINT)|(1<<TWEA);		//--- Acknowledgment Condition as per Datasheet
	while (!(TWCR & (1<<TWINT)));				//--- Wait until Acknowledgment Condition is transmitted to Slave
	while(TW_STATUS != TW_MR_DATA_ACK);			//--- Check for Acknowledgment 						
	usart_msg("Receiving MR data ACK ");		//--- Feedback msg to check error
	usart_tx(0x0D);								//--- Next Line
	return TWDR;								//--- Return received data from Slave
}

/* Function to Send Read No Acknowledgment */

char twi_read_nack()
{
	TWCR=(1<<TWEN)|(1<<TWINT);					//--- No Acknowledgment Condition as per Datasheet
	while (!(TWCR & (1<<TWINT)));				//--- Wait until No Acknowledgment Condition is transmitted to Slave
	while(TW_STATUS != TW_MR_DATA_NACK);		//--- Check for Acknowledgment
	usart_msg("Receiving MR Data NACK");		//--- Feedback msg to check error
	usart_tx(0x0D);								//--- Next Line
	return TWDR;								//--- Return received data
}

/* Function to Initialize USART */

void usart_init()
{
	UBRRH = 0;										//--- USART Baud Rate is set to 115200
	UBRRL = 0x08;	
	UCSRC = (1<<URSEL) | (1<<UCSZ1) | (1<<UCSZ0);	//--- 8-Bit Data Selected
	UCSRB = (1<<TXEN) | (1<<RXEN);					//--- Enable TX & RX
}

/* Function to Transmit data */

void usart_tx(char x)
{
	while (!( UCSRA & (1<<UDRE)));					//--- Check for Buffer is empty
	UDR = x;										//--- Send data to USART Buffer
}

/* Function to Receive data */

unsigned char usart_rx()
{
	while(!(UCSRA & (1<<RXC)));						//--- Check for data received completed
	return(UDR);									//--- Return the received data
}

/* Function to transmit string */

void usart_msg(char *c)
{
	while(*c != '\0')								//--- Check for Null
	usart_tx(*c++);									//--- Send the String
}


/****** END of Program ******/