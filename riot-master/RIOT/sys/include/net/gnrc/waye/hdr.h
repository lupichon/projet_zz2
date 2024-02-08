/*
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
 * @brief       Header definition of WAYE
 * @internal
 * @author      Raphael Bidaud  <bidaudr@gmail.com>
 */

#ifndef NET_GNRC_WAYE_HDR_H
#define NET_GNRC_WAYE_HDR_H

#include <stdint.h>
#include <stdbool.h>

#include "net/ieee802154.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Data frame type
 */
#define GNRC_WAYE_FRAMETYPE_DATA			(0U)

/**
 * @brief   WAYE (neighbor discovery request packet) frame type
 */
#define GNRC_WAYE_FRAMETYPE_WAYE			(1U)

/**
 * @brief   HELLO (neighbor discovery reply packet) frame type
 */
#define GNRC_WAYE_FRAMETYPE_HELLO			(2U)

/**
 * @brief   HELLO ACK frame type
 */
#define GNRC_WAYE_FRAMETYPE_HELLO_ACK		(3U)

/**
 * @brief   START COLLECT frame type
 */
#define GNRC_WAYE_FRAMETYPE_START_COLLECT	(4U)

/**
 * @brief   DATA POLL frame type
 */
#define GNRC_WAYE_FRAMETYPE_DATA_POLL		(5U)

/**
 * @brief   DATA REPLY frame type
 */
#define GNRC_WAYE_FRAMETYPE_DATA_REPLY		(6U)

/**
 * @brief   DATA ACK frame type
 */
#define GNRC_WAYE_FRAMETYPE_DATA_ACK		(7U)


/**
 * @brief   DATA REPLY queue size
 */
#define GNRC_WAYE_DATA_REPLY_QUEUE_SIZE		(10U)


/**
 * @brief	ACTUATOR ORDER frame type
*/
#define GNRC_WAYE_FRAMETYPE_ACTUATOR_ORDER	(11U)

/**
 * @brief	MIXED POLL frame type
*/
#define	GNRC_WAYE_FRAMETYPE_MIXED_POLL		(12U)

/**
 * @brief Number of redundancy in an order
*/
#ifndef GNRC_WAYE_ORDER_REDUNDANCY
#define GNRC_WAYE_ORDER_REDUNDANCY              (3U)
#endif

/**
 * @brief   type of node
 */
//#define GNRC_WAYE_NODE_ACTION		(21U)


/**
 * @brief   WAYE header
 */
typedef struct __attribute__((packed)) {
	uint8_t type;
} gnrc_waye_hdr_t;

/**
 * @brief   WAYE (neighbor discovery request packet) frame
 */
typedef struct __attribute__((packed)) {
	uint8_t cycle;
	uint8_t discovery_id; // ???
	uint8_t sector;
	uint8_t round;
	uint8_t nb_slot;
	float p;
	uint8_t slot_size_ms;
	//le_uint16_t discoID;
	//Liste *discoveredNodes;
	//le_uint16_t discovered_addr_list[];
} gnrc_waye_frame_waye_t;

/**
 * @brief   HELLO (neighbor discovery reply packet) frame
 */
typedef struct __attribute__((packed)) {
	uint8_t discovery_id; // ???
	uint8_t sector;
	uint8_t round;
	uint8_t selected_slot;
	int8_t rssi_waye;
	uint8_t lqi_waye;
	le_uint16_t addr;
	uint8_t node_type;
} gnrc_waye_frame_hello_t;

/**
 * @brief   HELLO ACK frame
 */
typedef struct __attribute__((packed)) {
	uint8_t discovery_id;
	uint8_t sector;
	uint8_t round;
	//le_uint16_t addr_list[]; //(variable size)
} gnrc_waye_frame_hello_ack_t;

/**
 * @brief   WAYE START COLLECT frame
 */
typedef struct __attribute__((packed)) {
	uint16_t delay_ms;
} gnrc_waye_frame_start_collect_t;

/**
 * @brief   WAYE DATA POLL frame
 */
typedef struct __attribute__((packed)) {
	uint8_t poll_id;
	uint8_t slot_size_ms;
//	le_uint16_t addr_list[]; (variable size)
} gnrc_waye_frame_data_poll_t;

/**
 * @brief   WAYE DATA MIXED POLL frame
 */
typedef struct __attribute__((packed)) {
	uint8_t poll_id;
	uint8_t slot_size_ms;
	uint8_t list_poll_size;
//	le_uint16_t addr_list[]; (variable size)
} gnrc_waye_frame_data_mixed_poll_t;

/**
 * @brief   WAYE data queue element
 */
typedef struct __attribute__((packed)) {
	uint8_t data;
	uint16_t timestamp;
} gnrc_waye_data_queue_element_t;

/**
 * @brief   WAYE DATA REPLY frame
 */
typedef struct __attribute__((packed)) {
	//uint8_t frame_control_field[2];
	uint8_t seq_num;
	uint8_t src_addr[2];
	uint8_t poll_id;
	uint8_t selected_slot;
	uint8_t data_queue_len;
	gnrc_waye_data_queue_element_t data_queue[GNRC_WAYE_DATA_REPLY_QUEUE_SIZE];
} gnrc_waye_frame_data_reply_t;

/**
 * @brief   WAYE DATA REPLY frame
 */
typedef struct __attribute__((packed)) {
	//uint8_t frame_control_field[2];
	uint8_t seq_num;
	uint8_t src_addr[2];
	uint8_t poll_id;
	uint8_t selected_slot;
	uint8_t data_queue_len;
} gnrc_waye_frame_data_dynamic_reply_t;

/**
 * @brief   WAYE DATA ACK frame
 */
typedef struct __attribute__((packed)) {
	uint8_t poll_id;
//	uint8_t bitmap[]; (variable size)
} gnrc_waye_frame_data_ack_t;

/**
 * @brief	WAYE ACTUATOR ORDER element
*/
typedef struct __attribute((packed)) {
	le_uint16_t addr;
	uint16_t order_id;
	uint8_t order;
	uint16_t parameters;
} gnrc_waye_frame_actuator_order_t;

/**
 * @brief   WAYE data queue element
 */
typedef struct __attribute__((packed)) {
	unsigned char data_golayed[6];
} gnrc_waye_data_golayed_element_t;

/**
 * @brief   WAYE DATA REPLY frame
 */
typedef struct __attribute__((packed)) {
	//uint8_t frame_control_field[2];
	gnrc_waye_data_golayed_element_t data_queue[GNRC_WAYE_DATA_REPLY_QUEUE_SIZE+2];
} gnrc_waye_frame_data_golayed_t;


#ifdef __cplusplus
}
#endif

#endif /* NET_GNRC_WAYE_HDR_H */
/** @} */
