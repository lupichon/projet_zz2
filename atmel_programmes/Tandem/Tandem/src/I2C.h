/**
 * @file I2C.h
 * @authors PICHON Lucas (Lucas.PICHON@etu.isima.fr) & ROBIN Igor (Igor.ROBIN@etu.isima.fr)
 * @brief Définitions et déclarations pour le module I2C
 * @version 1.0
 * @date 2024-02-25
 * 
 * 
 */

#ifndef I2C_H_
#define I2C_H_

#include <asf.h>
#include <i2c_slave.h>
#include <i2c_master.h>


#define SLAVE_ADDRESS 0x12  /* Adresse de l'esclave I2C */
#define DATA_LENGTH 10      /* Longueur des données */

extern struct i2c_master_module i2c_master_instance;    /* Instance du module maître I2C */
extern struct i2c_slave_module i2c_slave_instance;      /* Instance du module esclave I2C */

/**
 * @brief Enumération des adresses utilisées dans le système I2C
 */
enum addresses {
    MY_ADDRESS,         /* Adresse de cet appareil */
    SLAVE_1_ADDRESS,    /* Adresse de l'esclave 1 */
};

/**
 * @brief Enumération des informations de paquet I2C
 */
enum packet_info {
    MSG_TYPE,       /* Type de message */
    DATA,           /* Données */
    DATA_LEN,       /* Longueur des données */
    NO_DATA = 0xFF  /* Pas de données */
};

extern uint8_t write_buffer_master[DATA_LENGTH];    /* Tampon d'écriture pour le maître I2C */
extern uint8_t write_buffer_slave[DATA_LENGTH];     /* Tampon d'écriture pour l'esclave I2C */

extern uint8_t read_buffer_master[DATA_LENGTH];     /* Tampon de lecture pour le maître I2C */
extern uint8_t read_buffer_slave[DATA_LENGTH];      /* Tampon de lecture pour l'esclave I2C */

extern struct i2c_master_packet packet_master;      /* Paquet maître I2C */
extern struct i2c_slave_packet packet_slave;        /* Paquet esclave I2C */

/**
 * @brief Enumération des messages utilisés dans le système I2C
 */
enum messages {
    I_AM_MASTER = 0xA0, /* Message : Je suis le maître */
    I_AM_SLAVE = 0xA1,  /* Message : Je suis l'esclave */
    I_AM_READY = 0xA2,  /* Message : Je suis prêt */
    YES = 0xA3,         /* Message : Oui */
    INFO_MSG = 0xA4,    /* Message d'information */
    NO_INFO = 0xA5      /* Pas d'information */
};

/**
 * @brief Configure l'interface Maître I2C
 */
void configure_i2c_master(void);

/**
 * @brief Configure l'interface Esclave I2C
 */
void configure_i2c_slave(void);

#endif 