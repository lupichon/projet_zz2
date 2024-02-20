// Gardien
#ifndef MASTER_H_
#define MASTER_H_

// Inclusions
#include <asf.h>
#include "I2C.h"

// Election du maitre
bool master_election(void);

// Envoi du maitre a l'esclave
void send_master(enum messages msg_type, uint8_t data);

#endif /* MASTER_H_ */