/**
 * @file Slave.c
 * @authors PICHON Lucas (Lucas.PICHON@etu.isima.fr) & ROBIN Igor (Igor.ROBIN@etu.isima.fr)
 * @brief Fonctions de gestion des esclaves
 * @version 1.0
 * @date 2024-02-25
 */

#include "Slave.h"

/**
 * @brief Lecture d'un message
 *
 * Cette fonction permet aux eslaves de lire un message sur le bus et de déterminer si le maître est toujours présent
 *
 * @param msg_type Le type de message à lire.
 * 
 * @return Un booléen permettant de savoir si le maître à planté
 */
uint8_t read_slave(enum messages msg_type)
{
    tc_set_count_value(&tc_instance, 0);	/* Réinitialiser le compteur du timer/counter */
    tc_start_counter(&tc_instance); 		/* Démarrer le compteur du timer */
    uint32_t counter = 0; 					/* Compteur pour mesurer le temps écoulé */
    uint32_t bug = 0; 					

    packet_slave.data = read_buffer_slave; 

    /* Attendre jusqu'à ce que le type de message lu corresponde au type de message spécifié ou que le temps défini soit écoulé */
    while (counter < PERIODE && read_buffer_slave[MSG_TYPE] != msg_type)
    {
        i2c_slave_read_packet_wait(&i2c_slave_instance, &packet_slave); 	/* Lire le paquet depuis le maître */
        counter = tc_get_count_value(&tc_instance); 						/* Mettre à jour le compteur de temps écoulé */
    }

    read_buffer_slave[MSG_TYPE] = NO_INFO; 	/*Réinitialiser le type de message dans le tampon de lecture */
    tc_stop_counter(&tc_instance); 			/* Arrêter le compteur du timer */

    if (counter >= PERIODE) /* Vérifier si le temps défini est écoulé */
    {
        bug = 1; 
    }

    return bug; 
}

/**
 * @brief Envoi d'un message
 * 
 * Cette fonction permet aux esclaves d'envoyer un message sur le bus I2C
 *
 * @param msg_type Le type de message à envoyer.
 * @param data Les données à envoyer (facultatif).
 */
void write_slave(enum messages msg_type, uint8_t data)
{
    uint8_t master_address = read_buffer_slave[DATA]; 
    write_buffer_slave[MSG_TYPE] = msg_type; 
    write_buffer_slave[DATA] = data; 

    packet_slave.data = write_buffer_slave; /* Assigner les données au paquet */

    enum status_code status = STATUS_BUSY; /* Variable pour stocker le statut de l'écriture */

    /* Attendre que l'écriture du paquet soit terminée */
    while (status != STATUS_OK)
    {
        status = i2c_slave_write_packet_wait(&i2c_slave_instance, &packet_slave);
    }
}

/**
 * @brief Renvoie l'identifiant unique des cartes
 *
 * @return L'identifiant unique des cartes
 */
unsigned int unique_id(void)
{
    uint32_t word0 = *(volatile uint32_t *)WORD0_ADDRESS; /* Lire le mot 0 */
    uint32_t word1 = *(volatile uint32_t *)WORD1_ADDRESS; /* Lire le mot 1 */
    uint32_t word2 = *(volatile uint32_t *)WORD2_ADDRESS; /* Lire le mot 2 */
    uint32_t word3 = *(volatile uint32_t *)WORD3_ADDRESS; /* Lire le mot 3 */

    uint64_t high_part = ((uint64_t)word0 << 32) | word1; /* Concaténer les mots 0 et 1 pour obtenir la partie haute */
    uint64_t low_part = ((uint64_t)word2 << 32) | word3;  /* Concaténer les mots 2 et 3 pour obtenir la partie basse */

    unsigned int id = (unsigned int)high_part ^ (unsigned int)low_part; /**< Combiner les parties haute et basse pour obtenir l'identifiant unique */

    return id; 
}