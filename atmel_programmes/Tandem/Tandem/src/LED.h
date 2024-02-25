/**
 * @file LED.h
 * @authors PICHON Lucas (Lucas.PICHON@etu.isima.fr) & ROBIN Igor (Igor.ROBIN@etu.isima.fr)
 * @brief Définitions et déclarations pour la configuration de la LED
 * @version 1.0
 * @date 2024-02-25
 * 
 * 
 */

#ifndef LED_H_
#define LED_H_

#include <asf.h>

/**
 * @brief Configure la LED
 *
 * Cette fonction configure la broche de la LED en tant que sortie et initialise son niveau de sortie.
 */
void config_led(void);

/**
 * @brief Alias pour config_led()
 *
 * Cette fonction est un alias pour config_led(). 
 */
void conf_led(void);

#endif /* LED_H_ */