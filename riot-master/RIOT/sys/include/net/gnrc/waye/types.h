/**
 * Copyright (C) 2019 Raphael Bidaud
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     net_gnrc_waye
 * @{
 *
 * @file
 * @brief       Internal used types of WAYE
 * @internal
 * @author      Raphael Bidaud  <bidaudr@gmail.com>
 */

#ifndef NET_GNRC_WAYE_TYPES_H
#define NET_GNRC_WAYE_TYPES_H

#include <stdint.h>
#include <stdbool.h>

#include "xtimer.h"
#include "net/gnrc/waye/hdr.h"
#include "net/gnrc/pkt.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   WAYE round end event type.
 */
#define GNRC_WAYE_EVENT_ROUND_END	(0x4300)

/**
 * @brief   WAYE collect end event type.
 */
#define GNRC_WAYE_EVENT_COLLECT_END	(0x4301)

/**
 * @brief   WAYE init end event type.
 */
#define GNRC_WAYE_EVENT_INIT_END	(0x4302)

/**
 * @brief   WAYE specific structure for storing internal states.
 */
typedef struct waye {

} gnrc_waye_t;

/**
 * @brief   Structure for storing receive timestamps of HELLO packets
 */
typedef struct {
	gnrc_pktsnip_t *pkt;
	uint32_t rx_start_counter;
	uint32_t rx_end_counter;
} gnrc_waye_hello_counters_t;

/**
 * @brief	Structure to store the data received by the sink
*/
typedef struct {
	gnrc_waye_frame_data_reply_t pkt;
	int slot;
	uint8_t poll_id;
} gnrc_waye_data_counters_t;

/**
 * @brief   Enumeration for the state of a slot
 */
typedef enum {
	SLOT_EMPTY = 0,		/* Empty slot */
	SLOT_SUCCESS = 1,	/* One node replied in the slot */
	SLOT_COLLISION = 2,	/* At least two nodes replied in the slot */
} gnrc_waye_slot_state_t;

/**
 * @brief Structure for detecting the activity and the energy in a slot
 */
typedef struct {
	bool busy;
	uint8_t energylvl;
} gnrc_waye_slot_activity_t;

/**
 * @brief Structure used to chose the next node(s) in the collect phase
 */
typedef struct gnrc_waye_waiting_node {
	uint8_t frame_type;
	uint8_t node_sector;
	le_uint16_t node_addr;
	gnrc_waye_frame_actuator_order_t actuator_order;
	struct gnrc_waye_waiting_node *next;
} gnrc_waye_waiting_node_t;

#ifdef __cplusplus
}
#endif

#endif /* NET_GNRC_WAYE_TYPES_H */
/** @} */
