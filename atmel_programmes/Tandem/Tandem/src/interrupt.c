#include "interrupt.h"


void irq_handler(void)
{
	
	system_reset();
}


void init_irq_interrupt(void)
{
	struct extint_chan_conf config_extint_chan;
	extint_chan_get_config_defaults(&config_extint_chan);
	config_extint_chan.gpio_pin = IRQ_PIN;									//numero de broche associé à l'interruption
	config_extint_chan.gpio_pin_mux = MUX_PA07A_EIC_EXTINT7;				//configuration de la broche en canal d'entrée
	config_extint_chan.gpio_pin_pull = EXTINT_PULL_DOWN;					//résistance de tirage
	config_extint_chan.detection_criteria = EXTINT_DETECT_RISING;			//citère de détéction de l'interruption
	config_extint_chan.filter_input_signal = true;
	extint_chan_set_config(CANAL, &config_extint_chan);							//configuration du canal d'entrée grâce à la structure
	extint_register_callback(irq_handler,CANAL, EXTINT_CALLBACK_TYPE_DETECT);	//permet de définir la fonction callback appelée à chaque interruption
	extint_chan_enable_callback(CANAL, EXTINT_CALLBACK_TYPE_DETECT);			//activer la détéction d'interruptions
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

uint8_t val = 0;
void tc_callback_heartbeat(struct tc_module *const module_inst)
{
	int bug = rand()%1000;
	if(1)
	{
		port_pin_set_output_level(TIMER_PIN,val);
		val = !val;
		//port_pin_set_output_level(LED0_PIN,LED_0_INACTIVE);
	}
	else
	{
		port_pin_set_output_level(LED0_PIN,LED_0_INACTIVE);
		delay_ms(500);
		port_pin_set_output_level(LED0_PIN,LED_0_ACTIVE);
	}
}void configure_tc(void)
{
	struct tc_config config_tc;
	tc_get_config_defaults(&config_tc);
	config_tc.counter_size = TC_COUNTER_SIZE_8BIT;
	config_tc.clock_source = GCLK_GENERATOR_1;
	config_tc.clock_prescaler = TC_CLOCK_PRESCALER_DIV1024;
	config_tc.counter_8_bit.period = PERIODE;
	config_tc.counter_8_bit.compare_capture_channel[0] = 50;
	config_tc.counter_8_bit.compare_capture_channel[1] = 54;
	tc_init(&tc_instance, CONF_TC_MODULE, &config_tc);
	tc_enable(&tc_instance);
}

void configure_tc_callbacks(void)
{
	tc_register_callback(&tc_instance, tc_callback_heartbeat,TC_CALLBACK_OVERFLOW);
	tc_enable_callback(&tc_instance, TC_CALLBACK_OVERFLOW);
}void configure_port_heartbeat(enum port_pin_dir direction){	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);
	pin_conf.direction = direction;
	port_pin_set_config(TIMER_PIN, &pin_conf);}uint8_t v = 0;void start_timer(void)
{
	if(v==0)
	{
		tc_set_count_value(&tc_instance,0);
		tc_start_counter(&tc_instance);
		//start_counter = tc_get_count_value(&tc_instance);
		//port_pin_set_output_level(LED0_PIN,LED_0_ACTIVE);
	}
	else
	{
		end_counter = tc_get_count_value(&tc_instance);
		if(end_counter == 0)
		{
			port_pin_set_output_level(LED0_PIN,LED_0_ACTIVE);
		}
		else
		{
			port_pin_set_output_level(LED0_PIN,LED_0_INACTIVE);port_pin_set_output_level(LED0_PIN,LED_0_INACTIVE);
		}
		tc_stop_counter(&tc_instance);
		//port_pin_set_output_level(LED0_PIN,LED_0_INACTIVE);
	}
	v= !v;
}void slave_interrupt(void){
	struct extint_chan_conf config_extint_chan;
	extint_chan_get_config_defaults(&config_extint_chan);
	config_extint_chan.gpio_pin = TIMER_PIN;									
	config_extint_chan.gpio_pin_mux = MUX_PA06A_EIC_EXTINT6;	
	config_extint_chan.gpio_pin_pull = EXTINT_PULL_DOWN;				
	config_extint_chan.detection_criteria = EXTINT_DETECT_BOTH;			
	config_extint_chan.filter_input_signal = true;
	extint_chan_set_config(CANAL_SLAVE, &config_extint_chan);							
	extint_register_callback(start_timer,CANAL_SLAVE, EXTINT_CALLBACK_TYPE_DETECT);	
	extint_chan_enable_callback(CANAL_SLAVE, EXTINT_CALLBACK_TYPE_DETECT);			
	}