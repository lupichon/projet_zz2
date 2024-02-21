#ifndef I2C_H_
#define I2C_H_

#include <asf.h>
#include <i2c_slave.h>
#include <i2c_master.h>


#define SLAVE_ADDRESS 0x12
#define DATA_LENGTH 10

extern struct i2c_master_module i2c_master_instance;
extern struct i2c_slave_module i2c_slave_instance;

enum addresses {
	MY_ADDRESS,
	SLAVE_1_ADDRESS,
};

enum packet_info {
	MSG_TYPE,
	DATA,
	DATA_LEN,
	NO_DATA = 0xFF
};

extern uint8_t write_buffer_master[DATA_LENGTH];
extern uint8_t write_buffer_slave[DATA_LENGTH];

extern uint8_t read_buffer_master[DATA_LENGTH];
extern uint8_t read_buffer_slave[DATA_LENGTH];

extern struct i2c_master_packet packet_master;
extern struct i2c_slave_packet packet_slave;


static uint8_t write_buffer[DATA_LENGTH] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09
};

enum messages {
	I_AM_MASTER = 0xA0,
	I_AM_SLAVE = 0xA1,
	I_AM_READY = 0xA2,
	YES = 0xA3,
	INFO_MSG = 0xA4,
	NO_INFO = 0xA5
};

void configure_i2c_master(void);
void configure_i2c_slave(void);

#endif 