#include <asf.h>
#include <delay.h>
#include <stdlib.h>
#include "I2C.h"
#include "interrupt.h"
#include "LED.h"
#include "Master.h"
#include "Slave.h"

#define NB_BOARD = 2;

uint8_t read_buffer_master[DATA_LENGTH];
uint8_t read_buffer_slave[DATA_LENGTH];

struct i2c_master_module i2c_master_instance;
struct i2c_slave_module i2c_slave_instance;
struct tc_module tc_instance;

struct i2c_master_packet packet_master = {
	.address = SLAVE_1_ADDRESS,
	.data_length = DATA_LENGTH,
	.data = write_buffer_master,
	.ten_bit_address = false,
	.high_speed = false,
	.hs_master_code = 0x00,
};

struct i2c_slave_packet packet_slave = {
	.data_length = DATA_LENGTH,
	.data = write_buffer_slave,
};

uint8_t write_buffer_master[DATA_LENGTH] = {
	NO_DATA, NO_DATA,
};

uint8_t write_buffer_slave[DATA_LENGTH] = {
	NO_DATA, NO_DATA,
};

int main (void)
{
	system_init();
	delay_init();
	config_led();
	configure_tc();
	configure_tc_callbacks();
	uint8_t run = 1;
	uint8_t i_am_master = 0;
	
	unsigned id = unique_id();
	srand(id);
	delay_ms(100);
	//port_pin_set_output_level(LED_0_PIN,LED_0_ACTIVE);
	i_am_master = master_election();

	while (run)
	{
		while(i_am_master)
		{
			//port_pin_set_output_level(LED_0_PIN,LED_0_ACTIVE);
			send_master(I_AM_MASTER,MY_ADDRESS);
		}
		while(!i_am_master)
		{
			read_slave(I_AM_MASTER);
			
			/*
			delay_ms(6000);
			send_interrupt();
			i_am_master = master_election();
			*/
		}
	}
}