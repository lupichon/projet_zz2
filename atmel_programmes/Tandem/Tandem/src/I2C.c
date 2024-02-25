/**
 * @file I2C.c
 * @authors PICHON Lucas (Lucas.PICHON@etu.isima.fr) & ROBIN Igor (Igor.ROBIN@etu.isima.fr)
 * @brief Configuration du module I2C 
 * @version 1.0
 * @date 2024-02-25
 * 
 * 
 */

#include "I2C.h"

/**
 * @brief Configurer l'interface Maître I2C
 *
 * Cette fonction configure l'interface maître I2C en mettant en place les configurations nécessaires
 * et en initialisant l'instance maître I2C avec les paramètres de configuration spécifiés.
 * Elle active également le module maître I2C après l'initialisation.
 */

void configure_i2c_master(void)
{
	
	struct i2c_master_config config_i2c_master; 
	
	/* Récupérer les paramètres de configuration par défaut pour le maître I2C */
	i2c_master_get_config_defaults(&config_i2c_master);
	
	/* Définir la configuration du pinmux pour l'interface maître I2C */
	config_i2c_master.pinmux_pad0 = PINMUX_PA16C_SERCOM1_PAD0; /* Configuration du pinmux pour SDA */
	config_i2c_master.pinmux_pad1 = PINMUX_PA17C_SERCOM1_PAD1; /* Configuration du pinmux pour SCL */
	
	/* Initialiser le maître I2C avec la configuration spécifiée */
	i2c_master_init(&i2c_master_instance, EXT1_I2C_MODULE, &config_i2c_master);

	 /* Activer le module maître I2C après l'initialisation */
	i2c_master_enable(&i2c_master_instance);
}

/**
 * @brief Configurer l'interface Esclave I2C
 *
 * Cette fonction configure l'interface esclave I2C en mettant en place les configurations nécessaires
 * et en initialisant l'instance esclave I2C avec les paramètres de configuration spécifiés.
 * Elle active également le module esclave I2C après l'initialisation.
 */

void configure_i2c_slave(void)
{
    struct i2c_slave_config config_i2c_slave; 

    /* Récupérer les paramètres de configuration par défaut pour l'esclave I2C */
    i2c_slave_get_config_defaults(&config_i2c_slave);

    /* Définir l'adresse et le mode d'adresse pour l'esclave I2C */
    config_i2c_slave.address = SLAVE_1_ADDRESS; /* Adresse de l'esclave I2C */
    config_i2c_slave.address_mode = I2C_SLAVE_ADDRESS_MODE_MASK; /* Mode d'adresse de l'esclave I2C */

    /* Définir la configuration du pinmux pour l'interface esclave I2C */
    config_i2c_slave.pinmux_pad0 = PINMUX_PA16C_SERCOM1_PAD0; /* Configuration du pinmux pour SDA */
    config_i2c_slave.pinmux_pad1 = PINMUX_PA17C_SERCOM1_PAD1; /* Configuration du pinmux pour SCL */

    /* Définir le délai de tampon pour l'esclave I2C */
    config_i2c_slave.buffer_timeout = 1000; 

    /* Initialiser l'esclave I2C avec la configuration spécifiée */
    i2c_slave_init(&i2c_slave_instance, EXT1_I2C_MODULE, &config_i2c_slave);

    /* Activer le module esclave I2C après l'initialisation */
    i2c_slave_enable(&i2c_slave_instance);
}

