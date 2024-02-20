// Gardien
#ifndef INTERRUPT_H_
#define INTERRUPT_H_

// Inclusions
#include <asf.h>
#include "I2C.h"
#include "Slave.h"

// Macros
#define IRQ_PIN 7
#define CANAL 7
#define ITR_PIN_MASTER PIN_PA07

// Gestion des interruptions
void irq_handler(void);
void init_irq_interrupt(void);
void init_irq_pin(void);

// Envoi de l'interruption de l'esclave vers le maitre
void send_interrupt(void);

#endif /* INTERRUPT_H_ */