#ifndef MASTER_H_
#define MASTER_H_

#include <asf.h>
#include "I2C.h"

uint8_t master_election(void);
void send_master(enum messages msg_type, uint8_t data);
void read_master(enum messages msg_type);


#endif /* MASTER_H_ */