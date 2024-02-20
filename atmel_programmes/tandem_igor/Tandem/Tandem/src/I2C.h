// Gardien
#ifndef I2C_H_
#define I2C_H_

// Inclusions
#include <asf.h>
#include <i2c_slave.h>
#include <i2c_master.h>

// Adresse de l'esclave (1 esclave pour l'instant)
#define SLAVE_1_ADDRESS 0x01

// Structure du paquet
enum packet_info {
	MSG_TYPE = 0,
	DATA = 1,
	DATA_LEN = 2,
	NO_DATA = 0xFF
};

// Types de messages envoyes et recus
enum messages {
	I_AM_MASTER = 0xA0,
	INFO_MSG = 0xA1
};

extern struct i2c_master_module i2c_master_instance;
extern struct i2c_slave_module i2c_slave_instance;

extern uint8_t write_buffer_master[DATA_LEN];
extern uint8_t read_buffer_slave[DATA_LEN];

extern struct i2c_master_packet packet_master;
extern struct i2c_slave_packet packet_slave;

// Configurations du maitre et de l'esclave
void configure_i2c_master(void);
void configure_i2c_slave(void);

#endif 