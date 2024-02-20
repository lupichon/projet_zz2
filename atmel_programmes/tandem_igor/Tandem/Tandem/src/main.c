// Bibliotheques issues de ASF
#include <asf.h>
#include <delay.h>

// Bibliotheques du langage C
#include <stdlib.h>
#include <stdbool.h>

// Fichiers crees
#include "I2C.h"
#include "interrupt.h"
#include "LED.h"
#include "Master.h"
#include "Slave.h"

// Signal et bug
#define SIG_LEN 20
#define BUG_INDEX 7
#define FALSE_INFO 42

// Delais LEDs
#define HUGE_BLINK 1000
#define SMALL_BLINK 300

// Delais lecture/ecriture et interruptions/changement maitre-esclave
#define INTERRUPT_DELAY 5000
#define WAIT_WRITE 1000

// Attente aleatoire election maitre debut
#define UPPER_RANDOM_DELAY 3000
#define LOWER_RANDOM_DELAY 1000

// Buffers de lecture de l'esclave et d'ecriture du maitre
uint8_t read_buffer_slave[DATA_LEN] = { NO_DATA, NO_DATA };
uint8_t write_buffer_master[DATA_LEN] = { NO_DATA, NO_DATA };

// Instances du maitre et de l'esclave
struct i2c_master_module i2c_master_instance;
struct i2c_slave_module i2c_slave_instance;

// Paquets envoyes et recus
// Paquet du maitre envoye
struct i2c_master_packet packet_master = {
	.address = SLAVE_1_ADDRESS,
	.data_length = DATA_LEN,
	.data = write_buffer_master,
	.ten_bit_address = false,
	.high_speed = false,
	.hs_master_code = 0x00,
};

// Paquet de l'eslcave recu
struct i2c_slave_packet packet_slave = {
	.data_length = DATA_LEN,
	.data = read_buffer_slave,
};

// Programme principal
int main (void)
{
	// Initialisation du systeme
	system_init();
	delay_init();
	config_led();
	
	// Signal connu du maitre et de l'esclave (comportement de l'application connu)
	uint8_t signal[SIG_LEN] = {0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4, 0, 1, 2, 3, 4};
		
	// Nous commecons a suivre l'application a son debut (0)
	uint8_t index = 0; 
	
	// Identifiant unique recupere pour chaque carte
	unsigned id = unique_id();
	
	// Attente aleatoire pour l'election de maitre au depart
	srand(id);
	delay_ms(rand() % UPPER_RANDOM_DELAY + LOWER_RANDOM_DELAY);
	
	// Booleens controlant le deroulement du programme et le role maitre ou esclave
	bool run = true;
	bool i_am_master = false;
	
	// Premiere election
	i_am_master = master_election();
	
	// Boucle principale
	while (run)
	{
		// Je suis maitre
		while(i_am_master)
		{
			if (signal[index] == 2) // On verifie que l'esclave devenu maitre reprend bien la ou l'ancien maitre s'etait arrete
			{						  // Si c'est le cas alors la nouveau maitre clignote longuement
				blink_led(HUGE_BLINK, 1);
			}
			
			// J'envoie je suis maitre
			send_master(I_AM_MASTER, 0x00);
			
			// Tant qu'il y a de l'information a lire
			while (index < SIG_LEN)
			{
				// J'attend que l'esclave soit pret a lire avant d'ecrire
				delay_ms(WAIT_WRITE);
				
				// Bug artificiel, a l'indice 7 du signal nosu envoyons une fausse information
				if (index == BUG_INDEX) // On envoie une fausse information synonyme de bug (a adapter pour un plantage avec les battements de coeur)
				{
					// Envoi de la fausse information et on sort e la boucle
					send_master(INFO_MSG, FALSE_INFO);
					break;
				}
				else
				{
					// Le reste du temps, on envoie les informations (vraies)
					send_master(INFO_MSG, signal[index]); // Si on veut que le tandem boucle, on garde uniquement cette ligne la dans le while (index < SIG_LEN) et (2)
				}
				index++; // (2)
			}
			//index = 0; Au cas ou on veut recommencer depuis le debut la lecture du signal (que le tandem boucle)
		}
		
		// Je suis esclave
		while(!i_am_master)
		{
			// Je lis "je suis maitre"
			read_slave(I_AM_MASTER);
			
			// Je vide mon buffer de lecture
			empty_read_buffer_slave();
			
			// Tant qu'il y a de l'information a lire
			while (index < SIG_LEN)
			{
				// Je lis les informations
				read_slave(INFO_MSG);
				
				// Pour chaque information lue, je clignote rapidement
				// Si la donnee lue est conforme a ce a quoi on s'attend, on continue de lire
				if (packet_slave.data[DATA] == signal[index])
				{
					blink_led(SMALL_BLINK, 1);
					index++;
					
					// Entre chaque information, je vide mon buffer
					empty_read_buffer_slave();
				}
				// Sinon on envoie une interruption au maitre
				else
				{
					// Attente avant interruption
					delay_ms(INTERRUPT_DELAY);
					
					// Envoi interruption
					send_interrupt();
					
					// Nouvelle election : je suis maitre
					i_am_master = master_election();
					break;
				}
			}
			//index = 0; Au cas ou on veut recommencer depuis le debut la lecture du signal (que le tandem boucle)
		}
	}
}

