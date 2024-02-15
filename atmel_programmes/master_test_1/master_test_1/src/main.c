#include <asf.h>
#include <delay.h>
#include <i2c_master.h>

#define DATA_LENGTH 10
#define SLAVE_ADDRESS 0x12
#define TIMEOUT 1000

#define IRQ_PIN 7
#define CANAL 7


int val = 0;
int bug = 0, nb = 0;
//premiere valeur : état (0 ou 1)
//deuxieme valeur : temps dans cet etat
static uint8_t write_buffer[DATA_LENGTH] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09
};

struct i2c_master_packet packet = {
	.address = SLAVE_ADDRESS,
	.data_length = DATA_LENGTH,
	.data = write_buffer,
	.ten_bit_address = false,
	.high_speed = false,
	.hs_master_code = 0x00,
};



static uint8_t read_buffer[DATA_LENGTH];

struct i2c_master_module i2c_master_instance;

static void configure_i2c_master(void)
{
	struct i2c_master_config config_i2c_master;
	
	i2c_master_get_config_defaults(&config_i2c_master);
	
	config_i2c_master.pinmux_pad0 = PINMUX_PA16C_SERCOM1_PAD0;
	config_i2c_master.pinmux_pad1 = PINMUX_PA17C_SERCOM1_PAD1;
	
	i2c_master_init(&i2c_master_instance, EXT1_I2C_MODULE, &config_i2c_master); //EDBG_I2C_MODULE
	i2c_master_enable(&i2c_master_instance);
	
}

static void config_led(void)
{
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);
	
	pin_conf.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(LED_0_PIN, &pin_conf);
	port_pin_set_output_level(LED_0_PIN, LED_0_INACTIVE);
}

static void onde(void)
{
	/*
	PM->APBCMASK.reg |= PM_APBCMASK_TC3; 
	GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID_TCC0_TCC1 << GCLK_CLKCTRL_ID_Pos | GCLK_CLKCTRL_GEN_GCLK1 << GCLK_CLKCTRL_GEN_Pos | GCLK_CLKCTRL_CLKEN;
	
	TC3->COUNT16.CTRLA.reg &= ~TC_CTRLA_ENABLE;

	TC3->COUNT16.CTRLA.reg |= TC_CTRLA_MODE_COUNT16; // Mode 16 bits
	TC3->COUNT16.CTRLA.reg |= TC_CTRLA_WAVEGEN_MFRQ; // Mode de fonctionnement : fréquence de mesure
	TC3->COUNT16.CTRLA.reg |= TC_CTRLA_PRESCALER_DIV1024; // Diviseur de fréquence : 1024
	
	TC3->COUNT16.CC[0].reg = (uint16_t)(SystemCoreClock / 1024 / 1000 * TIMER_PERIOD_MS);
	TC3->COUNT16.INTENSET.reg = TC_INTENSET_OVF;
	NVIC_EnableIRQ(TC3_IRQn);
	TC3->COUNT16.CTRLA.reg |= 1;
	uint32_t start_counter = timer_counter;
	*/
	if(!bug)
	{
		val = !val;
		port_pin_set_output_level(10,val);
		packet.data[0] = val;
		int v = port_pin_get_output_level(10);
		/*if(v == 0)
		{
			port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
		}
		else
		{
			port_pin_set_output_level(LED_0_PIN,LED_0_INACTIVE);
		}*/
		delay_ms(500);
	}/*
	nb++;
	if(nb==3)
	{
		bug = 1;
	}*/
}

static void blink_led(uint8_t nb_blinks)
{
	for (int i = 0; i < nb_blinks; i++)
	{
		port_pin_set_output_level(LED_0_PIN, LED_0_INACTIVE);
		delay_ms(500);
		port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
		delay_ms(500);
	}
}

void irq_handler(void)
{
	/*
	blink_led(5);
	port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
	delay_ms(1000);*/
	system_reset();
}

//initialisation de la borche à laquelle on envoit des interruptions
void init_irq_interrupt(void)
{
 	struct extint_chan_conf config_extint_chan;
 	extint_chan_get_config_defaults(&config_extint_chan);
	config_extint_chan.gpio_pin = IRQ_PIN;									//numero de broche associé à l'interruption
	config_extint_chan.gpio_pin_mux = MUX_PA07A_EIC_EXTINT7;				//configuration de la broche en canal d'entrée
	config_extint_chan.gpio_pin_pull = EXTINT_PULL_DOWN;					//résistance de tirage
	//config_extint_chan.detection_criteria = EXTINT_DETECT_RISING;			//citère de détéction de l'interruption
	config_extint_chan.filter_input_signal = true;
	extint_chan_set_config(CANAL, &config_extint_chan);							//configuration du canal d'entrée grâce à la structure
	extint_register_callback(irq_handler,CANAL, EXTINT_CALLBACK_TYPE_DETECT);	//permet de définir la fonction callback appelée à chaque interruption
	extint_chan_enable_callback(CANAL, EXTINT_CALLBACK_TYPE_DETECT);			//activer la détéction d'interruptions		
																			//EXTINT_CALLBACK_TYPE_DETECT -> structure qui permet de configurer la condition d'interruption 
}

int main (void)
{
	system_init();
	delay_init();
	configure_i2c_master();
	config_led();
	init_irq_interrupt();
	system_interrupt_enable_global();
	uint16_t timeout = 0;
	//delay_ms(3000);
	//port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
	while(1)
	{
		/*onde();
		while (i2c_master_write_packet_wait(&i2c_master_instance, &packet) != STATUS_OK)
		{
			if (timeout++ == TIMEOUT)
			{
				break;
			}
		}
		if (timeout < TIMEOUT)
		{
			//blink_led(5);
		}
		else
		{
			//blink_led(1);
		}*/
		int test = port_pin_get_input_level(IRQ_PIN);
		if(test==true)
		{
			//port_pin_set_output_level(LED_0_PIN, LED_0_ACTIVE);
		}
		else
		{
			//port_pin_set_output_level(LED_0_PIN, LED_0_INACTIVE);
		}
	}	
	/*
	packet.data = read_buffer;
	while (i2c_master_read_packet_wait(&i2c_master_instance, &packet) != STATUS_OK)
	{
		if (timeout++ == TIMEOUT)
		{
			break;
		}
	}*/
}



/*
Le puits est chargé de produire sur un autre bus (appelons le bus de sortie) une forme d'onde basique : un signal rectangulaire à une fréquence à définir.
Donc le Maître pilote un niveau 1 pendant un temps T1 et un niveau 0 pendant un temps T1
Il indique périodiquement  aux exclaves l'état où il en est (niveau 1 ou niveau 0) et le temps consommé dans cet état (via le bus entre les cartes).
Puis à l'aide d'une fonction aléatoire il stoppe son activité et les esclaves entrent en jeu.... */
