/**
 * @file Master.h
 * @authors PICHON Lucas (Lucas.PICHON@etu.isima.fr) & ROBIN Igor (Igor.ROBIN@etu.isima.fr)
 * @brief Définitions et déclarations pour la gestion du maître
 * @version 1.0
 * @date 2024-02-25
 * 
 * 
 */

#ifndef MASTER_H_
#define MASTER_H_

#include <asf.h>
#include "I2C.h"

/**
 * @brief Élection du maître
 *
 * Cette fonction effectue le processus d'élection du maître dans le réseau.
 *
 * @return Le statut de l'élection : 1 si le maître est élu, 0 sinon.
 */
uint8_t master_election(void);

/**
 * @brief Envoi d'un message 
 *
 * Cette fonction permet au maître d'envoyer un message sur le bus I2C
 *
 * @param msg_type Le type de message à envoyer.
 * @param data Les données à envoyer (facultatif).
 */
void send_master(enum messages msg_type, uint8_t data);

/**
 * @brief Lecture d'un message
 *
 * Cette fonction permet de lire un message sur le bus I2C
 *
 * @param msg_type Le type de message à lire.
 */
void read_master(enum messages msg_type);

#endif /* MASTER_H_ */