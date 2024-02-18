#ifndef INTERRUPT_H_
#define INTERRUPT_H_

#include <asf.h>
#include "I2C.h"
#include "Slave.h"

#define IRQ_PIN 7
#define CANAL 7
#define ITR_PIN_MASTER PIN_PA07

void irq_handler(void);
void init_irq_interrupt(void);
void init_irq_pin(void);
void send_interrupt(void);



#endif /* INTERRUPT_H_ */