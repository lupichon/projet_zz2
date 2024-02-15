#include <asf.h>
#include <i2c_slave.h>
#include <i2c_master.h>
#include <delay.h>
#include <rtc.h>
#include <conf_clocks.h>
//#include <clock.h>
#include <stdlib.h>
#include <clock.h>
//#include <rtc.h>
//#define SLAVE_ADDRESS 0x12
#define TIMEOUT 1000
#define INFO 0x44

#define TIMEOUT_BIS 30

struct rtc_module rtc_instance;

void configure_rtc_count(void)
{
	struct rtc_count_config config_rtc_count;
	rtc_count_get_config_defaults(&config_rtc_count);
	config_rtc_count.prescaler = RTC_COUNT_PRESCALER_DIV_1;
	config_rtc_count.mode = RTC_COUNT_MODE_16BIT;
	#ifdef FEATURE_RTC_CONTINUOUSLY_UPDATED
	config_rtc_count.continuously_update = true;
	#endif
	config_rtc_count.compare_values[0] = 1000;
	rtc_count_init(&rtc_instance, RTC, &config_rtc_count);
	rtc_count_enable(&rtc_instance);
}

enum packet_info {
	MSG_TYPE,
	DATA,
	DATA_LENGTH,
	NO_DATA = 0xFF,
	};
	
enum messages {
	I_AM_MASTER = 0xA0,
	I_AM_SLAVE = 0xA1,
	I_AM_READY = 0xA2,
	YES = 0xA3,
	INFO_MSG = 0xA4,
	};

enum addresses {
	MY_ADDRESS,
	SLAVE_1_ADDRESS,
	};

static uint8_t write_buffer_master[DATA_LENGTH] = {
	NO_DATA, NO_DATA,
};

static uint8_t write_buffer_slave[DATA_LENGTH] = {
	NO_DATA, NO_DATA,
};

static uint8_t read_buffer_master[DATA_LENGTH];
static uint8_t read_buffer_slave[DATA_LENGTH];

struct i2c_slave_module i2c_slave_instance;
struct i2c_master_module i2c_master_instance;

static void configure_i2c_master(void)
{
	struct i2c_master_config config_i2c_master;
	
	i2c_master_get_config_defaults(&config_i2c_master);
	
	config_i2c_master.pinmux_pad0 = PINMUX_PA16C_SERCOM1_PAD0;
	config_i2c_master.pinmux_pad1 = PINMUX_PA17C_SERCOM1_PAD1;
	
	i2c_master_init(&i2c_master_instance, EXT1_I2C_MODULE, &config_i2c_master);
	i2c_master_enable(&i2c_master_instance);
	
}

static void configure_i2c_slave(void)
{
	struct i2c_slave_config config_i2c_slave;
	
	i2c_slave_get_config_defaults(&config_i2c_slave);
	
	config_i2c_slave.address = SLAVE_1_ADDRESS;
	config_i2c_slave.address_mode = I2C_SLAVE_ADDRESS_MODE_MASK;
	
	config_i2c_slave.pinmux_pad0 = PINMUX_PA16C_SERCOM1_PAD0;
	config_i2c_slave.pinmux_pad1 = PINMUX_PA17C_SERCOM1_PAD1;
	
	config_i2c_slave.buffer_timeout = 1000;
	
	i2c_slave_init(&i2c_slave_instance, EXT1_I2C_MODULE, &config_i2c_slave);
	i2c_slave_enable(&i2c_slave_instance);
}

static void config_led(void)
{
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);
	
	pin_conf.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LED_0_PIN, &pin_conf);
	port_pin_set_output_level(LED_0_PIN, LED_0_INACTIVE);
}


int main (void)
{
	system_init();
	delay_init();
	config_led();
	configure_rtc_count();
	//rtc_count_set_period(&rtc_instance, 2000);
	
	srand(2);
	
	delay_ms(5000);
	//uint16_t random_number = (rand() % 500) + 500;
	
	for (uint8_t i = 0; i < 10; i++)
	{
		 port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
		 delay_ms((rand() % 3000) + 50);
		 port_pin_set_output_level(LED_0_PIN, LED_0_INACTIVE);
		 delay_ms(500);
	}
	
	uint16_t timeout = 0;
	uint8_t run = 1;
	uint8_t i_am_master = 0;
	
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
	
	enum status_code status = STATUS_BUSY;
	//enum i2c_slave_direction dir;
	
	delay_ms(2000);
	
	configure_i2c_slave();
	//configure_i2c_master();
	
	while (run)
	{
		//configure_i2c_slave();
		packet_slave.data = read_buffer_slave;
		enum i2c_slave_direction dir = I2C_SLAVE_DIRECTION_READ;
		while (dir != I2C_SLAVE_DIRECTION_NONE)
		{
			dir = i2c_slave_get_direction_wait(&i2c_slave_instance);
			i_am_master = 1;
			if (dir == I2C_SLAVE_DIRECTION_READ)
			{
				//port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
				i_am_master = 0;
				break;
			}
		}
		
		if (dir == I2C_SLAVE_DIRECTION_NONE)
		{
			//port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
		}
		
		if (i_am_master)
		{
			//port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
			i2c_slave_disable(&i2c_slave_instance);
			//port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
			configure_i2c_master();
			//packet_master.address = SLAVE_1_ADDRESS;
			write_buffer_master[MSG_TYPE] = I_AM_MASTER;
			write_buffer_master[DATA] = MY_ADDRESS;
			packet_master.data = write_buffer_master;
			
			//port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
			while (status != STATUS_OK)
			{
				//port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
				status = i2c_master_write_packet_wait(&i2c_master_instance, &packet_master);
				//port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
			}
			
			//port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
			
			//while (1)
			//{
				
			//}
			//delay_ms(500);
			
			//port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
			//status = STATUS_BUSY;
			//packet_master.data = read_buffer;
			//while (read_buffer[MSG_TYPE] != I_AM_SLAVE)
			//{
			//	i2c_master_read_packet_wait(&i2c_master_instance, &packet_master);
			//}
			
			status = STATUS_BUSY;
			packet_master.data = read_buffer_master;
			while (read_buffer_master[MSG_TYPE] != I_AM_READY)
			{
				i2c_master_read_packet_wait(&i2c_master_instance, &packet_master);
			}
			
			status = STATUS_BUSY;
			write_buffer[MSG_TYPE] = INFO_MSG;
			write_buffer[DATA] = INFO;
			packet_master.data = write_buffer;
			
			//port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
			/*
			while (INFO)
			{
				i2c_master_write_packet_wait(&i2c_master_instance, &packet_master);
			}
			*/
		}
		else
		{
			//port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
			//port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
			//port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
			//configure_i2c_slave();
			packet_slave.data = read_buffer_slave;
			while (read_buffer_slave[MSG_TYPE] != I_AM_MASTER)
			{
				i2c_slave_read_packet_wait(&i2c_slave_instance, &packet_slave);
			}
			
			//port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
			//while (1)
			//{
				
			//}
			/*
			uint8_t master_address = read_buffer_slave[DATA];
			
			write_buffer[MSG_TYPE] = I_AM_SLAVE;
			write_buffer[DATA] = NO_DATA;
			packet_slave.data = write_buffer;
			
			while (status != STATUS_OK)
			{
				status = i2c_slave_write_packet_wait(&i2c_slave_instance, &packet_slave);
			}
			*/
			
			
			
			write_buffer_slave[MSG_TYPE] = I_AM_READY;
			write_buffer_slave[DATA] = NO_DATA;
			packet_slave.data = write_buffer_slave;
			
			while (status != STATUS_OK)
			{
				status = i2c_slave_write_packet_wait(&i2c_slave_instance, &packet_slave);
			}
			*/
			
			//uint8_t listen = 1;
			
			//port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
			
			//while (1)
			//{
				
			//}
			
			//while (listen)
			//{
				/*
				while (status != STATUS_OK)
				{
					i2c_slave_read_packet_wait(&i2c_slave_instance, &packet_slave);
					if (timeout++ == TIMEOUT)
					{
						break;
					}
				}
				if (timeout == TIMEOUT)
				{
					//send_interrupt
					// listen = 0;
				}
				*/
				
			//}
		}
	}
}
