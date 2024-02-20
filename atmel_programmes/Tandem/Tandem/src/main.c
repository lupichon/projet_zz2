#include <asf.h>
#include <delay.h>
#include <stdlib.h>
#include "I2C.h"
#include "interrupt.h"
#include "LED.h"
#include "Master.h"
#include "Slave.h"
#include <time.h>
#include <math.h>

#define NB_BOARD = 2;

uint8_t read_buffer_master[DATA_LENGTH];
uint8_t read_buffer_slave[DATA_LENGTH];

uint8_t start_counter = 0;
uint8_t end_counter = 0;

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
	config_led();	system_interrupt_enable_global();
	uint8_t run = 1;
	uint8_t i_am_master = 0;
	
	unsigned id = unique_id();
	srand(id);
	delay_ms((rand()%10000)+1);
	i_am_master = master_election();

	while (run)
	{
		while(i_am_master)
		{
			send_master(I_AM_MASTER,MY_ADDRESS);
			port_pin_set_output_level(LED0_PIN,LED_0_ACTIVE);
		}
		while(!i_am_master)
		{
			read_slave(I_AM_MASTER);
			while(1)
			{
				if(end_counter==0)
				{
					//port_pin_set_output_level(LED0_PIN,LED_0_ACTIVE);
					//send_interrupt();
					i_am_master = master_election();
				}
				else
				{
					//port_pin_set_output_level(LED0_PIN,LED_0_INACTIVE);
				}
			}
			/*if(port_pin_get_input_level(TIMER_PIN)==1)
			{
				tc_set_count_value(&tc_instance,0);
				tc_start_counter(&tc_instance);
				uint32_t avant = tc_get_count_value(&tc_instance);
				uint32_t apres = tc_get_count_value(&tc_instance);
				while(port_pin_get_input_level(TIMER_PIN)==1 && abs(apres-avant) <= 98)
				{
					apres = tc_get_count_value(&tc_instance);
				}
				tc_stop_counter(&tc_instance);
				if(apres-avant>98)
				{
					port_pin_set_output_level(LED0_PIN,LED_0_ACTIVE);
					delay_ms(500);
					port_pin_set_output_level(LED0_PIN,LED_0_INACTIVE);
					delay_ms(500);
					send_interrupt();
					i_am_master = master_election();
				}
			}
			else
			{
				tc_set_count_value(&tc_instance,0);
				tc_start_counter(&tc_instance);
				uint32_t avant = tc_get_count_value(&tc_instance);
				uint32_t apres = tc_get_count_value(&tc_instance);
				while(port_pin_get_input_level(TIMER_PIN)==0 && abs(apres-avant) <=98)
				{
					apres = tc_get_count_value(&tc_instance);
				}
				tc_stop_counter(&tc_instance);
				if(apres-avant>98)
				{
					port_pin_set_output_level(LED0_PIN,LED_0_ACTIVE);
					delay_ms(500);
					port_pin_set_output_level(LED0_PIN,LED_0_INACTIVE);
					delay_ms(500);
					send_interrupt();
					i_am_master = master_election();
				}
			}*/
		}
	}
}