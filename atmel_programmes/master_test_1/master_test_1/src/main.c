#include <asf.h>
#include <delay.h>
#include <i2c_master.h>

#define DATA_LENGTH 10
#define SLAVE_ADDRESS 0x12
#define TIMEOUT 1000


static uint8_t write_buffer[DATA_LENGTH] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
};

static uint8_t read_buffer[DATA_LENGTH];

struct i2c_master_module i2c_master_instance;

static void configure_i2c_master(void)
{
	struct i2c_master_config config_i2c_master;
	
	i2c_master_get_config_defaults(&config_i2c_master);
	
	config_i2c_master.pinmux_pad0 = PINMUX_PA16C_SERCOM1_PAD0;
	config_i2c_master.pinmux_pad1 = PINMUX_PA17C_SERCOM1_PAD1;
	
	i2c_master_init(&i2c_master_instance, EXT1_I2C_MODULE, &config_i2c_master); //EDBG_I2C_MODULE
	i2c_master_enable(&i2c_master_instance);
	
}

static void config_led(void)
{
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);
	
	pin_conf.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LED_0_PIN, &pin_conf);
	port_pin_set_output_level(LED_0_PIN, LED_0_INACTIVE);
}

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

int main (void)
{
	system_init();
	delay_init();
	configure_i2c_master();
	config_led();
	
	uint16_t timeout = 0;
	
	struct i2c_master_packet packet = {
		.address = SLAVE_ADDRESS,
		.data_length = DATA_LENGTH,
		.data = write_buffer,
		.ten_bit_address = false,
		.high_speed = false,
		.hs_master_code = 0x00,
	};
	
	while (i2c_master_write_packet_wait(&i2c_master_instance, &packet) != STATUS_OK)
	{
		if (timeout++ == TIMEOUT)
		{
			break;
		}
	}
	
	if (timeout < TIMEOUT)
	{
		blink_led(5);
	}

	packet.data = read_buffer;
	while (i2c_master_read_packet_wait(&i2c_master_instance, &packet) != STATUS_OK)
	{
		if (timeout++ == TIMEOUT)
		{
			break;
		}
	}
}
