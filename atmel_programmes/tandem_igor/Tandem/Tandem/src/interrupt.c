#include "interrupt.h"


void irq_handler(void)
{
	
	system_reset();
}

void init_irq_interrupt(void)
{
	struct extint_chan_conf config_extint_chan;
	extint_chan_get_config_defaults(&config_extint_chan);
	config_extint_chan.gpio_pin = IRQ_PIN;									//numero de broche associ� � l'interruption
	config_extint_chan.gpio_pin_mux = MUX_PA07A_EIC_EXTINT7;				//configuration de la broche en canal d'entr�e
	config_extint_chan.gpio_pin_pull = EXTINT_PULL_DOWN;					//r�sistance de tirage
	config_extint_chan.detection_criteria = EXTINT_DETECT_RISING;			//cit�re de d�t�ction de l'interruption
	config_extint_chan.filter_input_signal = true;
	extint_chan_set_config(CANAL, &config_extint_chan);							//configuration du canal d'entr�e gr�ce � la structure
	extint_register_callback(irq_handler,CANAL, EXTINT_CALLBACK_TYPE_DETECT);	//permet de d�finir la fonction callback appel�e � chaque interruption
	extint_chan_enable_callback(CANAL, EXTINT_CALLBACK_TYPE_DETECT);			//activer la d�t�ction d'interruptions
	//EXTINT_CALLBACK_TYPE_DETECT -> structure qui permet de configurer la condition d'interruption
}

void init_irq_pin(void)
{
	struct port_config config_port;
	port_get_config_defaults(&config_port);
	config_port.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(ITR_PIN_MASTER, &config_port);
	port_pin_set_output_level(ITR_PIN_MASTER, false);
}

 void send_interrupt(void)
{
	port_pin_set_output_level(ITR_PIN_MASTER, true);
	delay_us(50);
	port_pin_set_output_level(ITR_PIN_MASTER, false);
}