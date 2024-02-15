#include <asf.h>
#include <i2c_slave.h>
#include <i2c_master.h>
#include <delay.h>
#include <stdlib.h>
#define TIMEOUT 1000
#define INFO 0x44


uint8_t infos[20] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20};
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
	
	delay_ms(2000);
	
	configure_i2c_slave();
	
	uint8_t info_progression = 0;
	
	while (run)
	{
		packet_slave.data = read_buffer_slave;
		enum i2c_slave_direction dir = I2C_SLAVE_DIRECTION_READ;
		while (dir != I2C_SLAVE_DIRECTION_NONE)
		{
			dir = i2c_slave_get_direction_wait(&i2c_slave_instance);
			i_am_master = 1;
			if (dir == I2C_SLAVE_DIRECTION_READ)
			{
				i_am_master = 0;
				break;
			}
		}
		
		if (i_am_master)
		{
			if (infos[info_progression] == 14)
			{
				port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
			}
			i2c_slave_disable(&i2c_slave_instance);
			configure_i2c_master();

			write_buffer_master[MSG_TYPE] = I_AM_MASTER;
			write_buffer_master[DATA] = MY_ADDRESS;
			packet_master.data = write_buffer_master;
			
			while (status != STATUS_OK)
			{
				status = i2c_master_write_packet_wait(&i2c_master_instance, &packet_master);
			}
			
			status = STATUS_BUSY;
			packet_master.data = read_buffer_master;
			while (read_buffer_master[MSG_TYPE] != I_AM_READY)
			{
				i2c_master_read_packet_wait(&i2c_master_instance, &packet_master);
			}
			// jusque la ca marche
			write_buffer_master[MSG_TYPE] = INFO_MSG;
			while (info_progression < 20)
			{
				write_buffer_master[DATA] = infos[info_progression];
				packet_master.data = write_buffer_master;
				if (info_progression < 14) // bug
				{
					
					while (i2c_master_write_packet_wait(&i2c_master_instance, &packet_master) != STATUS_OK)
					{
						
					}
					info_progression++;
				}	
			}
		}
		else
		{
			packet_slave.data = read_buffer_slave;
			while (read_buffer_slave[MSG_TYPE] != I_AM_MASTER)
			{
				i2c_slave_read_packet_wait(&i2c_slave_instance, &packet_slave);
			}
			
			uint8_t master_address = read_buffer_slave[DATA];
			write_buffer_slave[MSG_TYPE] = I_AM_READY;
			write_buffer_slave[DATA] = NO_DATA;
			packet_slave.data = write_buffer_slave;
			
			while (status != STATUS_OK)
			{
				status = i2c_slave_write_packet_wait(&i2c_slave_instance, &packet_slave);
			}
			// ca marche jusque la
			uint8_t listen = 1;
			while (listen)
			{
				dir = i2c_slave_get_direction_wait(&i2c_slave_instance);
				
				if (dir == I2C_SLAVE_DIRECTION_READ)
				{
					packet_slave.data = read_buffer_slave;
					i2c_slave_read_packet_wait(&i2c_slave_instance, &packet_slave);
					if (packet_slave.data[DATA] == infos[info_progression])
					{
						info_progression++;
					}
					else
					{
						break;
					}
				}
			}
		}
	}
}
