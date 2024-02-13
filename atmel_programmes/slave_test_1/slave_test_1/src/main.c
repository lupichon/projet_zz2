#include <asf.h>
#include <i2c_slave.h>
#include <delay.h>

#define DATA_LENGTH 10
#define SLAVE_ADDRESS 0x12
#define TIMEOUT 1000

static uint8_t write_buffer[DATA_LENGTH] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
};

static uint8_t read_buffer[DATA_LENGTH];

struct i2c_slave_module i2c_slave_instance;

static void config_led(void)
{
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);
	
	pin_conf.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LED_0_PIN, &pin_conf);
	port_pin_set_output_level(LED_0_PIN, LED_0_INACTIVE);
}

/*
static void blink_led(uint8_t nb_blinks)
{
	for (int i = 0; i < nb_blinks; i++)
	{
		port_pin_set_output_level(LED_0_PIN, LED_0_INACTIVE);
		delay_ms(500);
		port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
		delay_ms(500);
	}
}
*/

static void configure_i2c_slave(void)
{
	struct i2c_slave_config config_i2c_slave;
	
	i2c_slave_get_config_defaults(&config_i2c_slave);
	
	config_i2c_slave.address = SLAVE_ADDRESS;
	config_i2c_slave.address_mode = I2C_SLAVE_ADDRESS_MODE_MASK;
	
	config_i2c_slave.pinmux_pad0 = PINMUX_PA16C_SERCOM1_PAD0;
	config_i2c_slave.pinmux_pad1 = PINMUX_PA17C_SERCOM1_PAD1;
	
	config_i2c_slave.buffer_timeout = 1000;
	
	i2c_slave_init(&i2c_slave_instance, EXT1_I2C_MODULE, &config_i2c_slave);
	i2c_slave_enable(&i2c_slave_instance);
}

//configuration de la broche utilisée pour générer des interruptions
void init_irq_pin(void)
{
	struct port_config config_port;
	port_get_config_defaults(&config_port);
	config_port.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(ITR_PIN_MASTER, &config_port);
	port_pin_set_output_level(ITR_PIN_MASTER, false);
}

//envoie d'une interrutpion
void send_interrupt(void)
{
	for (uint8_t i = 0; i < 110; i++)
	{
	port_pin_set_output_level(ITR_PIN_MASTER, true);		//motif à détecter pour la carte cible
	delay_us(50);
	port_pin_set_output_level(ITR_PIN_MASTER, false);
	delay_us(50);
	}
}


int main (void)
{
	system_init();
	
	config_led();
	
	configure_i2c_slave();
	
	enum i2c_slave_direction dir;
	
	struct i2c_slave_packet packet = {
		.data_length = DATA_LENGTH,
		.data = write_buffer,
	};
	
	while (1)
	{
		dir = i2c_slave_get_direction_wait(&i2c_slave_instance);
		
		if (dir == I2C_SLAVE_DIRECTION_READ)
		{
			packet.data = read_buffer;
			i2c_slave_read_packet_wait(&i2c_slave_instance, &packet);
			if (packet.data[4] == 0x04)
			{
				port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
			}
		}
		else if (dir == I2C_SLAVE_DIRECTION_WRITE)
		{
			packet.data = write_buffer;
			i2c_slave_write_packet_wait(&i2c_slave_instance, &packet);
		}
	}
}
