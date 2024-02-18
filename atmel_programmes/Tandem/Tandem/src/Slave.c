#include "Slave.h"

void read_slave(uint8_t msg_type)
{
	packet_slave.data = read_buffer_slave;
	while (read_buffer_slave[MSG_TYPE] != msg_type)
	{
		i2c_slave_read_packet_wait(&i2c_slave_instance, &packet_slave);
	}
}

void write_slave(uint8_t msg_type, uint8_t data)
{
	uint8_t master_address = read_buffer_slave[DATA];
	write_buffer_slave[MSG_TYPE] = msg_type;
	write_buffer_slave[DATA] = data;
	packet_slave.data = write_buffer_slave;
	enum status_code status = STATUS_BUSY; 
	
	while (status != STATUS_OK)
	{
		status = i2c_slave_write_packet_wait(&i2c_slave_instance, &packet_slave);
	}
}

unsigned int unique_id(void)
{
	uint32_t word0 = *(volatile uint32_t *)WORD0_ADDRESS;
	uint32_t word1 = *(volatile uint32_t *)WORD1_ADDRESS;
	uint32_t word2 = *(volatile uint32_t *)WORD2_ADDRESS;
	uint32_t word3 = *(volatile uint32_t *)WORD3_ADDRESS;

	uint64_t high_part = ((uint64_t)word0 << 32) | word1;
	uint64_t low_part = ((uint64_t)word2 << 32) | word3;

	unsigned int id = (unsigned int)high_part ^ (unsigned int)low_part;
	
	return id;
}
