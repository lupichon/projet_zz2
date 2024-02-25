/**
 * @file interrupt.c
 * @authors PICHON Lucas (Lucas.PICHON@etu.isima.fr) & ROBIN Igor (Igor.ROBIN@etu.isima.fr)
 * @brief Fonctions pour la gestion des interruptions 
 * @version 1.0
 * @date 2024-02-25
 * 
 * 
 */

#include "interrupt.h"


/**
 * @brief Gestionnaire d'interruption IRQ
 *
 * Cette fonction réinitialise le système lorsqu'une interruption est déclenchée.
 */
void irq_handler(void)
{
    system_reset(); 
}

/**
 * @brief Initialise l'interruption IRQ (maître)
 */
void init_irq_interrupt(void)
{
    struct extint_chan_conf config_extint_chan; 								/* Configuration du canal d'interruption externe */
    extint_chan_get_config_defaults(&config_extint_chan);

    config_extint_chan.gpio_pin = IRQ_PIN; 										/* Spécifier le numéro de la broche GPIO */
    config_extint_chan.gpio_pin_mux = MUX_PA07A_EIC_EXTINT7; 					/* Spécifier le multiplexeur de la broche GPIO */
    config_extint_chan.gpio_pin_pull = EXTINT_PULL_DOWN; 						/* Activer la résistance de pull-down */
    config_extint_chan.detection_criteria = EXTINT_DETECT_RISING; 				/* Définir le critère de détection sur le front montant */
    config_extint_chan.filter_input_signal = true; 								/* Activer le filtrage du signal d'entrée */
    
    extint_chan_set_config(CANAL, &config_extint_chan); 						/* Configurer le canal d'interruption externe */
    extint_register_callback(irq_handler, CANAL, EXTINT_CALLBACK_TYPE_DETECT); 	/* Enregistrer le gestionnaire d'interruption */
    extint_chan_enable_callback(CANAL, EXTINT_CALLBACK_TYPE_DETECT); 			/* Activer le rappel d'interruption */
}

/**
 * @brief Initialise la broche d'interruption (esclaves)
 */
void init_irq_pin(void)
{
    struct port_config config_port;						/* Configuration de la broche */
    port_get_config_defaults(&config_port);

    config_port.direction = PORT_PIN_DIR_OUTPUT; 		/* Définir la direction de la broche comme sortie */
    port_pin_set_config(ITR_PIN_MASTER, &config_port); 	/* Configurer la broche */
    port_pin_set_output_level(ITR_PIN_MASTER, false); 	/* Mettre le niveau de sortie à bas */
}

/**
 * @brief Envoie une interruption
 */
void send_interrupt(void)
{
	/* Création d'un front montant sur la broche d'interruption */
    port_pin_set_output_level(ITR_PIN_MASTER, true); 
    delay_us(50); 
    port_pin_set_output_level(ITR_PIN_MASTER, false); 
}

/**
 * @brief Callback du timer pour le battement de coeur
 *
 * @param module_inst Instance du module timer/counter
 */
void tc_callback_heartbeat(struct tc_module *const module_inst)
{
    int bug = rand() % 1000; /* Simulation d'un bug */
    if (bug > 200)
    {
        send_master(I_AM_MASTER, 0x00); /* Envoie du battement de coeur sur le bus I2C */
    }
    else /* bug */
    {
        port_pin_set_output_level(LED0_PIN, LED_0_INACTIVE);
        delay_ms(1000); 
        port_pin_set_output_level(LED0_PIN, LED_0_ACTIVE); 
    }
}

/**
 * @brief Configure le timer
 */
void configure_tc(void)
{
    struct tc_config config_tc; 
    tc_get_config_defaults(&config_tc);

    config_tc.counter_size = TC_COUNTER_SIZE_8BIT; 			/* Taille du compteur */
    config_tc.clock_source = GCLK_GENERATOR_1; 				/* Source d'horloge */
    config_tc.clock_prescaler = TC_CLOCK_PRESCALER_DIV1024; /* Préscaleur d'horloge */
    config_tc.counter_8_bit.period = PERIODE; 				/* Période du compteur */

    tc_init(&tc_instance, CONF_TC_MODULE, &config_tc); 		/* Initialiser le timer */
    tc_enable(&tc_instance); 								/* Activer le timer */
}

/**
 * @brief Configure les rappels du timer
 */
void configure_tc_callbacks(void)
{
    tc_register_callback(&tc_instance, tc_callback_heartbeat, TC_CALLBACK_OVERFLOW); 	/* Enregistrer le rappel de débordement */
    tc_enable_callback(&tc_instance, TC_CALLBACK_OVERFLOW); 							/* Activer le rappel de débordement */
}