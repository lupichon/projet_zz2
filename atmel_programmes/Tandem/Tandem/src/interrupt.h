#ifndef INTERRUPT_H_
#define INTERRUPT_H_

#include <asf.h>
#include "I2C.h"
#include "Slave.h"
#include "Master.h"

#define IRQ_PIN 7
#define TIMER_PIN PIN_PA06
#define CANAL 7
#define CANAL_SLAVE 6
#define ITR_PIN_MASTER PIN_PA07
#define CONF_TC_MODULE TC3
#define PERIODE 100

extern struct tc_module tc_instance;
extern uint8_t time_1, time_2;

void irq_handler(void);
void init_irq_interrupt(void);
void init_irq_pin(void);
void send_interrupt(void);
void timer_callback(struct tc_module *const module_inst);
void configure_tc(void);
void configure_tc_callbacks(void);
void tc_callback_to_toggle_led(struct tc_module *const module_inst);
void configure_port_heartbeat(enum port_pin_dir direction);
void start_timer(void);void slave_interrupt(void);

#endif /* INTERRUPT_H_ */