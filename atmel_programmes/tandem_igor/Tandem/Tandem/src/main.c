#include <asf.h>
#include <delay.h>
#include <stdlib.h>
#include "I2C.h"
#include "interrupt.h"
#include "LED.h"
#include "Master.h"
#include "Slave.h"

uint8_t read_buffer_master[DATA_LEN];
uint8_t read_buffer_slave[DATA_LEN] = {NO_DATA, NO_DATA};

struct i2c_master_module i2c_master_instance;
struct i2c_slave_module i2c_slave_instance;

struct i2c_master_packet packet_master = {
	.address = SLAVE_1_ADDRESS,
	.data_length = DATA_LEN,
	.data = write_buffer_master,
	.ten_bit_address = false,
	.high_speed = false,
	.hs_master_code = 0x00,
};

struct i2c_slave_packet packet_slave = {
	.data_length = DATA_LEN,
	.data = write_buffer_slave,
};

uint8_t write_buffer_master[DATA_LEN] = {
	NO_DATA, NO_DATA,
};

uint8_t write_buffer_slave[DATA_LEN] = {
	NO_DATA, NO_DATA,
};


	

int main (void)
{
	system_init();
	delay_init();
	config_led();
	
	uint8_t echelon[10] = {0, 2, 0, 2, 0, 2, 0, 2, 0, 2};
		
	uint8_t run = 1;
	uint8_t i_am_master = 0;
	srand(2);
	delay_ms(rand() % 3000 + 1000);
	i_am_master = master_election();
	uint8_t indice = 0;
	
	while (run)
	{
		while(i_am_master)
		{
			send_master(I_AM_MASTER, MY_ADDRESS);
			while (indice < 10)
			{
				send_master(INFO_MSG, echelon[indice]);
				indice++;
			}
			indice = 0;
		}
		while(!i_am_master)
		{
			read_slave(I_AM_MASTER);
			
			while (indice < 10)
			{
				
				delay_ms(500);
				read_slave(INFO_MSG);
				if (read_buffer_slave[DATA] == echelon[indice])
				{
					
					port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
					delay_ms(300);
					port_pin_set_output_level(LED_0_PIN, LED_0_INACTIVE);
					delay_ms(300);
					
					
					indice++;
					//indice = indice % 2;
				}
				else
				{
					delay_ms(5000);
					send_interrupt();
					i_am_master = master_election();
					break;
				}
			}
			indice = 0;
		}
	}
}

