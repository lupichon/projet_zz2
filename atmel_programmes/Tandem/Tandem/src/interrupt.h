/**
 * @file interrupt.h
 * @authors PICHON Lucas (Lucas.PICHON@etu.isima.fr) & ROBIN Igor (Igor.ROBIN@etu.isima.fr)
 * @brief Définitions et déclarations pour la gestion des interruptions
 * @version 1.0
 * @date 2024-02-25
 * 
 * 
 */

#ifndef INTERRUPT_H_
#define INTERRUPT_H_

#include <asf.h>
#include "I2C.h"
#include "Slave.h"
#include "Master.h"

#define IRQ_PIN 7               /* Numéro de broche pour l'interruption */
#define TIMER_PIN PIN_PA06      /* Broche utilisée pour le timer */
#define CANAL 7                 /* Numéro de canal pour le timer */
#define ITR_PIN_MASTER PIN_PA07 /* Broche utilisée pour l'interruption maître */
#define CONF_TC_MODULE TC3      /* Module timer à utiliser */
#define PERIODE 100             /* Période du timer en ms */

extern struct tc_module tc_instance; /* Instance du module timer/counter */

/**
 * @brief Fonction de callback
 *
 * Cette fonction réinitialise le système lorsqu'une interruption est déclenchée.
 */
void irq_handler(void);

/**
 * @brief Initialise l'interruption IRQ (maître)
 */
void init_irq_interrupt(void);

/**
 * @brief Initialise la broche d'interruption (esclaves)
 */
void init_irq_pin(void);

/**
 * @brief Envoie d'une interruption
 */
void send_interrupt(void);

/**
 * @brief Callback du timer pour le battement de coeur 
 *
 * @param module_inst Instance du module timer
 */
void timer_callback(struct tc_module *const module_inst);

/**
 * @brief Configure le timer
 */
void configure_tc(void);

/**
 * @brief Configure les rappels du timer
 */
void configure_tc_callbacks(void);

/**
 * @brief Callback du timer pour le battement de coeur
 *
 * @param module_inst Instance du module timer/counter
 */
void tc_callback_heartbeat(struct tc_module *const module_inst);

#endif /* INTERRUPT_H_ */