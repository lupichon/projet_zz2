// Gardien
#ifndef SLAVE_H_
#define SLAVE_H_

// Inclusions
#include <asf.h>
#include "I2C.h"

// Adresses ou sont localisees l'identite de la carte
#define WORD0_ADDRESS 0x0080A00C
#define WORD1_ADDRESS 0x0080A040
#define WORD2_ADDRESS 0x0080A044
#define WORD3_ADDRESS 0x0080A048

// Lecture de la part de l'esclave
void read_slave(enum messages msg_type);

// Vidage du buffer de lecture
void empty_read_buffer_slave(void);

// Recuperation d'un identifiant unique
unsigned int unique_id(void);

#endif /* SLAVE_H_ */