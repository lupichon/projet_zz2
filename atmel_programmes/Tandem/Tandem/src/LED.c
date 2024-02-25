/**
 * @file LED.c
 * @authors PICHON Lucas (Lucas.PICHON@etu.isima.fr) & ROBIN Igor (Igor.ROBIN@etu.isima.fr)
 * @brief Configuration de la LED
 * @version 1.0
 * @date 2024-02-25
 * 
 * 
 */

#include "LED.h"

/* 
 * Configure la LED
 * 
 * Cette fonction configure la broche de la LED en tant que sortie et initialise son niveau de sortie.
 */
void config_led(void)
{
    struct port_config pin_conf; /* Configuration de la broche */

    /* Obtenir la configuration par défaut de la broche */
    port_get_config_defaults(&pin_conf);

    /* Définir la direction de la broche comme sortie */
    pin_conf.direction = PORT_PIN_DIR_OUTPUT;
    port_pin_set_config(LED_0_PIN, &pin_conf); /* Configuration de la broche de la LED */

    /* Initialiser le niveau de sortie de la broche */
    port_pin_set_output_level(LED_0_PIN, LED_0_INACTIVE); /* Niveau de sortie initial */
}

/*
 * Alias pour config_led()
 *
 * Cette fonction est un alias pour config_led(). Elle est fournie pour des raisons de compatibilité et de lisibilité.
 */
void conf_led(void)
{
    config_led(); /* Appel de la fonction de configuration de la LED */
}