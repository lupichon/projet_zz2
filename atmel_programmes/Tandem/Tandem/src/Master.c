/**
 * @file Master.c
 * @authors PICHON Lucas (Lucas.PICHON@etu.isima.fr) & ROBIN Igor (Igor.ROBIN@etu.isima.fr)
 * @brief Fonctions de gestion du maître
 * @version 1.0
 * @date 2024-02-25
 * 
 * 
 */

#include "Master.h"
#include "interrupt.h"


/**
 * @brief Élection du maître
 *
 * Cette fonction effectue le processus d'élection du maître dans le réseau.
 *
 * @return Le statut de l'élection : 1 si le maître est élu, 0 sinon.
 */

uint8_t master_election(void)
{
    i2c_slave_disable(&i2c_slave_instance); 
    i2c_slave_enable(&i2c_slave_instance); 
    configure_i2c_slave(); 
    uint8_t i_am_master = 0; 
    enum i2c_slave_direction dir = i2c_slave_get_direction_wait(&i2c_slave_instance); /* Récupérer la direction de la communication */

    if (dir == I2C_SLAVE_DIRECTION_NONE) /* Vérifier si il y a déjà un maître  */
    {
        init_irq_interrupt(); 						/* Initialiser les interruptions IRQ côté maître*/
        i2c_slave_disable(&i2c_slave_instance); 	/* Désactiver l'interface esclave */
        configure_i2c_master(); 					/* Configurer l'interface maître */
        i_am_master = 1; 							
        configure_tc(); 							/* Configurer le timer/counter */
        configure_tc_callbacks(); 					/* Configurer les rappels de timer/counter */
        send_master(I_AM_MASTER, 0x00); 			/* Prevenir que les autres que la carte est maître */
    }
    else /* Je suis esclave */
    {
        configure_tc(); 							/* Configurer le timer/counter */
        init_irq_pin();								/* Initialiser les interruptions IRQ côté esclave */
    }
    return i_am_master;
}

/**
 * @brief Envoi d'un message 
 *
 * Cette fonction permet au maître d'envoyer un message sur le bus I2C
 *
 * @param msg_type Le type de message à envoyer.
 * @param data Les données à envoyer (facultatif).
 */

void send_master(enum messages msg_type, uint8_t data)
{
    write_buffer_master[MSG_TYPE] = msg_type; 
    write_buffer_master[DATA] = data; 

    packet_master.data = write_buffer_master; 	/* Assigner les données au paquet */

    enum status_code status = STATUS_BUSY; 		/* Variable pour stocker le statut de l'écriture */

    /* Attendre que l'écriture du paquet soit terminée */
    while (status != STATUS_OK)
    {
        status = i2c_master_write_packet_wait(&i2c_master_instance, &packet_master);
    }
}

/**
 * @brief Lecture d'un message
 *
 * Cette fonction permet de lire un message sur le bus I2C
 *
 * @param msg_type Le type de message à lire.
 */
void read_master(enum messages msg_type)
{
    packet_master.data = read_buffer_master; /* Assigner le tampon de lecture au paquet */

    /* Attendre jusqu'à ce que le type de message lu corresponde au type de message spécifié */
    while (read_buffer_master[MSG_TYPE] != msg_type)
    {
        i2c_master_read_packet_wait(&i2c_master_instance, &packet_master);
    }
}
