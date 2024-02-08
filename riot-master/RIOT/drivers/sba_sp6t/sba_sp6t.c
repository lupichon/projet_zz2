/*
 * Copyright (C) 2019 Raphael Bidaud
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     drivers_sba_sp6t
 * @{
 *
 * @file
 * @brief       Implementation of SP6T antenna switch driver
 *
 * @author      Raphael Bidaud  <bidaudr@gmail.com>
 * @}
 */

#include "sba_sp6t.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

/* Look-up table for changing the beam order */
static const uint8_t beam_lut[] = { 1, 3, 5, 4, 2, 0 };

int sba_sp6t_init(sba_sp6t_t *dev, const sba_sp6t_params_t *params) {
	assert(dev && params);
	dev->params = *params;

	if (gpio_init(dev->params.control1, GPIO_OUT) != 0) {
		DEBUG("ERROR: [SP6T] Cannot set pin Control#1 to output mode\n");
		return -1;
	}

	if (gpio_init(dev->params.control2, GPIO_OUT) != 0) {
		DEBUG("ERROR: [SP6T] Cannot set pin Control#2 to output mode\n");
		return -1;
	}

	if (gpio_init(dev->params.control3, GPIO_OUT) != 0) {
		DEBUG("ERROR: [SP6T] Cannot set pin Control#3 to output mode\n");
		return -1;
	}

	/* Initialize to beam 0 */
	gpio_clear(dev->params.control1);
	gpio_clear(dev->params.control2);
	gpio_clear(dev->params.control3);

	return 0;
}

int sba_sp6t_set_beam(sba_sp6t_t *dev, uint8_t beam) {
	assert(dev);

	if (beam <= 5) {
		/* Use value in the look-up table */
		beam = beam_lut[beam];

		int control1 = (beam & 4) >> 2;
		int control2 = (beam & 2) >> 1;
		int control3 = (beam & 1);

		gpio_write(dev->params.control1, control1);
		gpio_write(dev->params.control2, control2);
		gpio_write(dev->params.control3, control3);
	}
	else {
		return -EINVAL;
	}

	return 0;
}

uint8_t sba_sp6t_get_beam(sba_sp6t_t *dev) {
	assert(dev);

	uint8_t beam = 0;

	if (gpio_read(dev->params.control1)) {
		beam += 4;
	}

	if (gpio_read(dev->params.control2)) {
		beam += 2;
	}

	if (gpio_read(dev->params.control3)) {
		beam ++;
	}

	/* Search beam number in the look-up table */
	for (uint8_t i=0; i<6; i++) {
		if (beam_lut[i] == beam) {
			beam = i;
			break;
		}
	}

	return beam;
}