// Gardien
#ifndef LED_H_
#define LED_H_

#include <asf.h>

// Gestion de la LED
// Configuration
void config_led(void);

// Clignotement
void blink_led(uint16_t duration, uint8_t nb_blinks);

#endif /* LED_H_ */