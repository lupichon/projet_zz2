#include <asf.h>
#include <delay.h>
#include <i2c_master.h>
#include <time.h>

#define DATA_LENGTH 2
#define SLAVE_ADDRESS 0x12
#define TIMEOUT 1000

#define VAL 0
#define PORT 10

//premiere valeur : état (0 ou 1)
//deuxieme valeur : temps dans cet etat
static uint8_t write_buffer[DATA_LENGTH] = {
	0x00, 0x00
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

static void config_autre_bus(void)
{
	struct port_config pin_conf;
	port_get_config_defaults(&pin_conf);
	pin_conf.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(PORT, &pin_conf);
	port_pin_set_output_level(PORT,VAL);
}

static void onde(void)
{
	VAL = !=VAL;
	port_pin_set_output_level(10,VAL);
	delay_ms(500);
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

//initialisation de la borche à laquelle on envoit des interruptions
/*
void init_irq_interrupt(void)
{
 	struct extint_chan_conf config_extint_chan;
 	extint_chan_get_config_defaults(&config_extint_chan);
	config_extint_chan.gpio_pin = IRQ_PIN;									//numero de broche associé à l'interruption
	config_extint_chan.gpio_pin_mux = MUX_PA22A_EIC_EXTINT6;				//configuration de la broche en canal d'entrée
	config_extint_chan.gpio_pin_pull = EXTINT_PULL_NONE;					//résistance de tirage
	config_extint_chan.detection_criteria = EXTINT_DETECT_RISING;			//citère de détéction de l'interruption
	config_extint_chan.filter_input_signal = true;
	extint_chan_set_config(6, &config_extint_chan);							//configuration du canal d'entrée grâce à la structure (6 = canal)
	extint_register_callback(irq_handler, 6, EXTINT_CALLBACK_TYPE_DETECT);	//permet de définir la fonction callback appelée à chaque interruption
	extint_chan_enable_callback(6, EXTINT_CALLBACK_TYPE_DETECT);			//activer la détéction d'interruptions
																			//EXTINT_CALLBACK_TYPE_DETECT -> structure qui permet de configurer la condition d'interruption 
}

void irq_handler(void)
{
	irq_count++;
	if(irq_count>=100)
	{
		irq_count = 0;
		DELAY = 0;
	}
}
*/

/*
int main (void)
{
	system_init();
	delay_init();
	configure_i2c_master();
	config_led();
	
	uint16_t timeout = 0;
	
	struct i2c_master_packet packet = {
		.address = SLAVE_ADDRESS,
		.data_length = DATA_LENGTH,
		.data = write_buffer,
		.ten_bit_address = false,
		.high_speed = false,
		.hs_master_code = 0x00,
	};
	
	while (i2c_master_write_packet_wait(&i2c_master_instance, &packet) != STATUS_OK)
	{
		if (timeout++ == TIMEOUT)
		{
			break;
		}
	}
	
	if (timeout < TIMEOUT)
	{
		blink_led(5);
	}

	packet.data = read_buffer;
	while (i2c_master_read_packet_wait(&i2c_master_instance, &packet) != STATUS_OK)
	{
		if (timeout++ == TIMEOUT)
		{
			break;
		}
	}
}*/

int main()
{
	system_init();
	delay_init();
	configure_i2c_master();
	config_autre_bus();

	struct i2c_master_packet packet = 
	{
		.address = SLAVE_ADDRESS,
		.data_length = DATA_LENGTH,
		.data = write_buffer,
		.ten_bit_address = false,
		.high_speed = false,
		.hs_master_code = 0x00,
	};

	while(1)
	{
		clock_t debut = clock();
		onde()
		.data[0] = //lecture du port 10;
		clock_t end = clock();
		.data[1] = (double)(end - begin) / CLOCKS_PER_SEC;

		while (i2c_master_write_packet_wait(&i2c_master_instance, &packet) != STATUS_OK)
		{
			if (timeout++ == TIMEOUT)
			{
				break;
			}
		}
	}
}

/*
Le puits est chargé de produire sur un autre bus (appelons le bus de sortie) une forme d'onde basique : un signal rectangulaire à une fréquence à définir.
Donc le Maître pilote un niveau 1 pendant un temps T1 et un niveau 0 pendant un temps T1
Il indique périodiquement  aux exclaves l'état où il en est (niveau 1 ou niveau 0) et le temps consommé dans cet état (via le bus entre les cartes).
Puis à l'aide d'une fonction aléatoire il stoppe son activité et les esclaves entrent en jeu.... */
