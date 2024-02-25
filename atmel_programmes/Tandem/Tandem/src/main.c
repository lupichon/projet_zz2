/**
 * @file main.c
 * @authors PICHON Lucas (Lucas.PICHON@etu.isima.fr) & ROBIN Igor (Igor.ROBIN@etu.isima.fr)
 * @brief Réalisation du tandem entre maître/esclave(s)
 * @version 1.0
 * @date 2024-02-25
 * 
 * 
 */

#include <asf.h>
#include <delay.h>
#include <stdlib.h>
#include "I2C.h"
#include "interrupt.h"
#include "LED.h"
#include "Master.h"
#include "Slave.h"
#include <time.h>
#include <math.h>

/* 
 * @file main.c
 * @brief Réalisation du tandem entre maître/esclave(s)
 * @version 1.0
 * @date 2024-02-25
 */

#include <asf.h>
#include <delay.h>
#include <stdlib.h>
#include "I2C.h"
#include "interrupt.h"
#include "LED.h"
#include "Master.h"
#include "Slave.h"
#include <time.h>
#include <math.h>

// Attente aléatoire avant l'élection du maître
#define UPPER_RANDOM_DELAY 10000 
#define LOWER_RANDOM_DELAY 1 

// Nombre de cartes
#define NB_BOARD 2 

uint8_t read_buffer_master[DATA_LENGTH]; 		/* Tampon de lecture du maître */
uint8_t read_buffer_slave[DATA_LENGTH]; 		/* Tampon de lecture de l'esclave */

struct i2c_master_module i2c_master_instance; 	/* Instance du module maître I2C */
struct i2c_slave_module i2c_slave_instance; 	/* Instance du module esclave I2C */
struct tc_module tc_instance; 					/* Instance du module timer/counter */

struct i2c_master_packet packet_master = {
    .address = SLAVE_1_ADDRESS,
    .data_length = DATA_LENGTH,
    .data = write_buffer_master,
    .ten_bit_address = false,
    .high_speed = false,
    .hs_master_code = 0x00,
}; /* Structure de paquet pour le maître I2C */

struct i2c_slave_packet packet_slave = {
    .data_length = DATA_LENGTH,
    .data = write_buffer_slave,
}; /* Structure de paquet pour l'esclave I2C */

uint8_t write_buffer_master[DATA_LENGTH] = {
    NO_DATA, NO_DATA,
}; /* Tampon d'écriture du maître */

uint8_t write_buffer_slave[DATA_LENGTH] = {
    NO_DATA, NO_DATA,
}; /* Tampon d'écriture de l'esclave */

int main(void)
{
    system_init(); 						/* Initialiser le système */
    delay_init(); 						/* Initialiser la temporisation */
    config_led(); 						/* Configurer les LEDs */
    system_interrupt_enable_global(); 	/* Activer les interruptions globales */
    uint8_t run = 1; 
    uint8_t i_am_master = 0; 			/* Indicateur de maître */

    tc_set_count_value(&tc_instance, 0); 	/* Définir la valeur du compteur à zéro */
    unsigned id = unique_id(); 			 	/* Obtenir un identifiant unique */
    srand(id); 								/* Initialiser le générateur de nombres aléatoires */
    delay_ms((rand() % UPPER_RANDOM_DELAY) + LOWER_RANDOM_DELAY); /* Attendre un délai aléatoire */
    i_am_master = master_election(); /* Élire le maître */

    while (run)
    {
        while (i_am_master)
        {
            port_pin_set_output_level(LED0_PIN, LED_0_ACTIVE); 
        }
        while (!i_am_master)
        {
            port_pin_set_output_level(LED0_PIN, LED_0_ACTIVE); 
            uint8_t bug = read_slave(I_AM_MASTER); /* Est-ce que le maître est toujours présent ? */
            port_pin_set_output_level(LED0_PIN, LED_0_INACTIVE);
            delay_ms(1000); 
            if (bug)
            {
                send_interrupt(); 					/* Envoyer une interruption pour réinitialiser le maître*/
                i_am_master = master_election(); 	/* Réélire le maître */
            }
        }
    }
}


//ma�tre allum� permanent, faire clignoter l'eclave rapidement lorsqu'il prend la main 