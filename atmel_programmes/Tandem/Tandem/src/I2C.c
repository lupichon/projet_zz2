#include "I2C.h"

void configure_i2c_master(void)
{
	struct i2c_master_config config_i2c_master;
	
	i2c_master_get_config_defaults(&config_i2c_master);
	
	config_i2c_master.pinmux_pad0 = PINMUX_PA16C_SERCOM1_PAD0;
	config_i2c_master.pinmux_pad1 = PINMUX_PA17C_SERCOM1_PAD1;
	
	i2c_master_init(&i2c_master_instance, EXT1_I2C_MODULE, &config_i2c_master);
	i2c_master_enable(&i2c_master_instance);
	
}

void configure_i2c_slave(void)
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

