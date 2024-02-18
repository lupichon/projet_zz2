#ifndef SLAVE_H_
#define SLAVE_H_

#include <asf.h>
#include "I2C.h"

void write_slave(uint8_t msg_type, uint8_t data);
void read_slave(uint8_t msg_type);




#endif /* SLAVE_H_ */