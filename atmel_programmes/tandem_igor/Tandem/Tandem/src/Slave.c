#include "Slave.h"

// Recuperation d'un identifiant unique par carte
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

// Lecture de la part de l'esclave
void read_slave(enum messages msg_type)
{
	packet_slave.data = read_buffer_slave;
	while (packet_slave.data[MSG_TYPE] != msg_type)
	{
		i2c_slave_read_packet_wait(&i2c_slave_instance, &packet_slave);
	}
}

// Vidage du buffer de lecture de l'esclave
void empty_read_buffer_slave(void)
{
	read_buffer_slave[DATA] = NO_DATA;
	read_buffer_slave[MSG_TYPE] = NO_DATA;
}
