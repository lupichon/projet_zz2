#include "LED.h"

// Configuration de la LED
void config_led(void)
{
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);
	
	pin_conf.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LED_0_PIN, &pin_conf);
	port_pin_set_output_level(LED_0_PIN, LED_0_INACTIVE);
}

// Clignotement de la LED avec parametres
void blink_led(uint16_t duration, uint8_t nb_blinks)
{
	for (uint8_t i = 0; i < nb_blinks; i++)
	{
		port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
		delay_ms(duration);
		port_pin_set_output_level(LED_0_PIN, LED_0_INACTIVE);
		delay_ms(duration);
	}
}