#include "Master.h"
#include "interrupt.h"
#include <stdbool.h>

// Election du maitre
bool master_election()
{
	i2c_slave_disable(&i2c_slave_instance);
	i2c_slave_enable(&i2c_slave_instance);
	configure_i2c_slave();	
	bool i_am_master = false;
	enum i2c_slave_direction dir = i2c_slave_get_direction_wait(&i2c_slave_instance);
	
	if(dir == I2C_SLAVE_DIRECTION_NONE)
	{
		init_irq_interrupt();
		i2c_slave_disable(&i2c_slave_instance);
		configure_i2c_master();
		i_am_master = true;
	}	
	else
	{
		init_irq_pin();
	}
	return i_am_master;
}

// Envoi du maitre a l'esclave
void send_master(enum messages msg_type, uint8_t data)
{
	write_buffer_master[MSG_TYPE] = msg_type;
	write_buffer_master[DATA] = data;
	packet_master.data = write_buffer_master;
	enum status_code status = STATUS_BUSY;
	while (status != STATUS_OK)
	{
		status = i2c_master_write_packet_wait(&i2c_master_instance, &packet_master);
	}
}
