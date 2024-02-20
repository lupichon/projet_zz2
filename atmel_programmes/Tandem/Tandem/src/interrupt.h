#ifndef INTERRUPT_H_
#define INTERRUPT_H_

#include <asf.h>
#include "I2C.h"
#include "Slave.h"

#define IRQ_PIN 7
#define CANAL 7
#define ITR_PIN_MASTER PIN_PA07
#define CONF_TC_MODULE TC3

extern struct tc_module tc_instance;

void irq_handler(void);
void init_irq_interrupt(void);
void init_irq_pin(void);
void send_interrupt(void);
void timer_callback(struct tc_module *const module_inst);
void configure_tc(void);
void configure_tc_callbacks(void);
void tc_callback_to_toggle_led(struct tc_module *const module_inst);



#endif /* INTERRUPT_H_ */