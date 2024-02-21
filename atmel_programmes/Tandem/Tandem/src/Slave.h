#ifndef SLAVE_H_
#define SLAVE_H_

#include <asf.h>
#include "I2C.h"
#include "interrupt.h"

#define WORD0_ADDRESS 0x0080A00C
#define WORD1_ADDRESS 0x0080A040
#define WORD2_ADDRESS 0x0080A044
#define WORD3_ADDRESS 0x0080A048

void write_slave(enum messages msg_type, uint8_t data);
uint8_t read_slave(enum messages msg_type);
unsigned int unique_id(void);



#endif /* SLAVE_H_ */