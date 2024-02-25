/**
 * @file Slave.h
 * @authors PICHON Lucas (Lucas.PICHON@etu.isima.fr) & ROBIN Igor (Igor.ROBIN@etu.isima.fr)
 * @brief Définitions et déclarations pour la gestion des esclaves
 * @version 1.0
 * @date 2024-02-25
 */

#ifndef SLAVE_H_
#define SLAVE_H_

#include <asf.h>
#include "I2C.h"
#include "interrupt.h"

/* Adresses de récupération de l'identifiant unique du processeur (4 mots de 32 bits)*/
#define WORD0_ADDRESS 0x0080A00C /**< Adresse pour le mot 0 */
#define WORD1_ADDRESS 0x0080A040 /**< Adresse pour le mot 1 */
#define WORD2_ADDRESS 0x0080A044 /**< Adresse pour le mot 2 */
#define WORD3_ADDRESS 0x0080A048 /**< Adresse pour le mot 3 */

/**
 * @brief Envoi d'un message
 * 
 * Cette fonction permet aux esclaves d'envoyer un message sur le bus I2C
 *
 * @param msg_type Le type de message à envoyer.
 * @param data Les données à envoyer (facultatif).
 */
void write_slave(enum messages msg_type, uint8_t data);

/**
 * @brief Lecture d'un message
 *
 * Cette fonction permet aux eslaves de lire un message sur le bus et de déterminer si le maître est toujours présent
 *
 * @param msg_type Le type de message à lire.
 * 
 * @return Un booléen permettant de savoir si le maître à planté
 */
uint8_t read_slave(enum messages msg_type);

/**
 * @brief Renvoie l'identifiant unique des cartes
 *
 * @return L'identifiant unique des cartes
 */
unsigned int unique_id(void);

#endif /* SLAVE_H_ */