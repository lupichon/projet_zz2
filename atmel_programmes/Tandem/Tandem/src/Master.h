#ifndef MASTER_H_
#define MASTER_H_

#include <asf.h>
#include "I2C.h"

uint8_t master_election();
void send_master(uint8_t msg_type, uint8_t data);
void read_master(uint8_t msg_type);


#endif /* MASTER_H_ */