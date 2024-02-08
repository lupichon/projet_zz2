/*
 * Copyright (C) 2019 Raphael Bidaud
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    drivers_sba_sp6t SP6T antenna switch driver
 * @ingroup     drivers_switched_beam_antennas
 * @brief       Single Pole Six-Throw antenna switch using 3 control pins
 *
 * @{
 *
 * @file
 * @brief       Implementation of SP6T antenna switch driver
 *
 * @author      Raphael Bidaud  <bidaudr@gmail.com>
 */

#ifndef SBA_SP6T_H
#define SBA_SP6T_H

#include "periph/gpio.h"
#include "errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   SP6T params
 */
typedef struct sba_sp6t_params {
	gpio_t control1;	/**< Control#1 GPIO pin */
	gpio_t control2;	/**< Control#2 GPIO pin */
	gpio_t control3;	/**< Control#3 GPIO pin */
} sba_sp6t_params_t;

/**
 * @brief   Switched-Beam Antenna SP6T device descriptor
 */
typedef struct sba_sp6t {
	sba_sp6t_params_t params; /**< device driver configuration */
} sba_sp6t_t;

/**
 * @brief   Initializes a Switched-Beam Antenna SP6T device
 *          Beam 0 is selected after initialization
 *
 * @param[in,out]   dev     Device descriptor
 * @param[in]       params  Device configuration
 *
 * @return  0 on success
 * @return -1 if error while initializing GPIO pins to output mode
 */
int sba_sp6t_init(sba_sp6t_t *dev, const sba_sp6t_params_t *params);

/**
 * @brief   Switches to a specific beam
 *
 * @param[in]   dev     Device descriptor
 * @param[in]   beam    Beam number between 0 and 5
 *
 * @return  0 on success
 * @return -INVAL if beam > 5
 */
int sba_sp6t_set_beam(sba_sp6t_t *dev, uint8_t beam);

/**
 * @brief   Returns the current beam number
 *
 * @param[in]   dev     Device descriptor
 *
 * @return Beam number
 */
uint8_t sba_sp6t_get_beam(sba_sp6t_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* SBA_SP6T_H */
/** @} */
