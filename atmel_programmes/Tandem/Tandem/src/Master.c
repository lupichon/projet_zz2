#include "Master.h"
#include "interrupt.h"


uint8_t master_election(void)
{
	i2c_slave_disable(&i2c_slave_instance);
	i2c_slave_enable(&i2c_slave_instance);
	configure_i2c_slave();	
	uint8_t i_am_master = 0;
	enum i2c_slave_direction dir = i2c_slave_get_direction_wait(&i2c_slave_instance);
	
	if(dir == I2C_SLAVE_DIRECTION_NONE)
	{
		init_irq_interrupt();
		i2c_slave_disable(&i2c_slave_instance);
		configure_i2c_master();
		i_am_master = 1;
		configure_tc();	
		configure_tc_callbacks();
		send_master(I_AM_MASTER,0x00);
	}	
	else
	{
		configure_tc();
		init_irq_pin();
	}
	return i_am_master;
}

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

void read_master(enum messages msg_type)
{
	packet_master.data = read_buffer_master;
	while (read_buffer_master[MSG_TYPE] != msg_type)
	{
		i2c_master_read_packet_wait(&i2c_master_instance, &packet_master);
	}
}
