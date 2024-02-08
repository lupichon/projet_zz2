/*
 * Copyright (C) 2019 Raphael Bidaud
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @defgroup    net_gnrc_waye WAYE
 * @ingroup     net_gnrc
 * @brief       A MAC protocol for switched-beam antennas
 *
 * ## WAYE implementation
 *
 * @{
 *
 * @file
 * @brief       Implementation of WAYE protocol
 *
 * @author      Raphael Bidaud  <bidaudr@gmail.com>
 */

#ifndef NET_GNRC_WAYE_WAYE_H
#define NET_GNRC_WAYE_WAYE_H

#include "timex.h"
#include "net/gnrc/netif.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Default number of reply slots for the first round
 */
#ifndef GNRC_WAYE_INITIAL_SLOTS
#define GNRC_WAYE_INITIAL_SLOTS					(6U)
#endif

/**
 * @brief Default slot size in milliseconds for replies
 */
#ifndef GNRC_WAYE_ND_SLOT_SIZE_MS
#define GNRC_WAYE_ND_SLOT_SIZE_MS				(5U)
#endif

/**
 * @brief Default slot size in microseconds for replies
 */
#ifndef GNRC_WAYE_ND_SLOT_SIZE_US
#define GNRC_WAYE_ND_SLOT_SIZE_US				(GNRC_WAYE_ND_SLOT_SIZE_MS * US_PER_MS)
#endif

/**
 * @brief Default offset for slots
 */
#ifndef GNRC_WAYE_ND_SLOT_OFFSET_US
#define GNRC_WAYE_ND_SLOT_OFFSET_US				(1700U)
#endif

/**
 * @brief Default slot size in milliseconds for replies
 * Tested empirically, 7 milliseconds is not enough for our algorithm to get succesfully executed
 */
#ifndef GNRC_WAYE_COLLECT_SLOT_SIZE_MS
#define GNRC_WAYE_COLLECT_SLOT_SIZE_MS			(9U)
#endif

/**
 * @brief Default slot size in microseconds for replies
 */
#ifndef GNRC_WAYE_COLLECT_SLOT_SIZE_US
#define GNRC_WAYE_COLLECT_SLOT_SIZE_US			(GNRC_WAYE_COLLECT_SLOT_SIZE_MS * US_PER_MS)
#endif

/**
 * @brief Default offset for slots
 */
#ifndef GNRC_WAYE_COLLECT_SLOT_OFFSET_US
#define GNRC_WAYE_COLLECT_SLOT_OFFSET_US		(1700U)
#endif

/**
 * @brief Default queue size for HELLO messages
 */
#ifndef GNRC_WAYE_HELLO_QUEUE_SIZE
#define GNRC_WAYE_HELLO_QUEUE_SIZE				(30U)
#endif

/**
 * @brief Default size for stored DATA messages
 */
#ifndef GNRC_WAYE_DATA_QUEUE_SIZE
#define GNRC_WAYE_DATA_QUEUE_SIZE				(12U)
#endif

/**
 * @brief Default queue size for RX_START timestamps of corrupted packets
 */
#ifndef GNRC_WAYE_RX_START_QUEUE_SIZE
#define GNRC_WAYE_RX_START_QUEUE_SIZE			(30U)
#endif

/**
 * @brief Default queue size for medium busy timestamps
 */
#ifndef GNRC_WAYE_BUSY_QUEUE_SIZE
#define GNRC_WAYE_BUSY_QUEUE_SIZE				(120U)
#endif

/**
 * @brief Size of the array containing addresses to acknowledge
 */
#ifndef GNRC_WAYE_ACK_ADDR_LIST_SIZE
#define GNRC_WAYE_ACK_ADDR_LIST_SIZE			(12U)
#endif

/**
 * @brief Number of sectors of the antenna
 */
#ifndef GNRC_WAYE_SECTORS
#define GNRC_WAYE_SECTORS						(6U)
#endif

/**
 * @brief Time between medium busy and RX_START for HELLO packets
 */
#ifndef GNRC_WAYE_BUSY_RX_START_US
#define GNRC_WAYE_BUSY_RX_START_US				(0U)//(427U)
#endif

/**
 * @brief Time between medium busy and RX_END for HELLO packets
 */
#ifndef GNRC_WAYE_BUSY_RX_END_US
#define GNRC_WAYE_BUSY_RX_END_US				(0U)//(1037U)
#endif

/**
 * @brief RSSI threshold for collision detection
 */
#ifndef GNRC_WAYE_RSSI_THRESHOLD
#define GNRC_WAYE_RSSI_THRESHOLD				(-85)
#endif

/**
 * @brief Number of RSSI values to average
 */
#ifndef GNRC_WAYE_RSSI_NUMBER
#define GNRC_WAYE_RSSI_NUMBER					(6U)
#endif

/**
 * @brief Maximum number of bytes in group ack bitmap
 */
#ifndef GNRC_WAYE_GACK_BITMAP_MAX_SIZE
#define GNRC_WAYE_GACK_BITMAP_MAX_SIZE			(32U)
#endif

/**
 * @brief Size of rssi tables, used to analyze the medium (choosed to have a wide enough sample)
 */
#ifndef GNRC_WAYE_RSSI_TABLES_SIZE
#define GNRC_WAYE_RSSI_TABLES_SIZE              (2048U)
#endif

/**
 * @brief Size of both waiting queues
 */
#ifndef GNRC_WAYE_WAITING_QUEUE_SIZE
#define GNRC_WAYE_WAITING_QUEUE_SIZE            (24U)
#endif

/**
 * @brief Number of redundancy in an order
*/
#ifndef GNRC_WAYE_ORDER_REDUNDANCY
#define GNRC_WAYE_ORDER_REDUNDANCY              (3U)
#endif


/**
 * @brief       Convert rtt ticks to microseconds
 * @param[in]   ticks   rtt ticks
 * @return              number of microseconds
 */
#define TICKS_TO_US(ticks)      ((uint32_t) RTT_TICKS_TO_US(ticks))

/**
 * @brief Nodes only connected to actuators
*/
#define GNRC_WAYE_NODE_TYPE_ACTUATOR            (0U)

/**
 * @brief Nodes only connected to captors
*/
#define GNRC_WAYE_NODE_TYPE_CAPTOR              (1U)

/**
 * @brief Nodes connected to actuators and captors
*/
#define GNRC_WAYE_NODE_TYPE_CAPTOR_ACTUATOR     (2U)

/**
 * @brief   Creates an IEEE 802.15.4 WAYE network interface
 *
 * @param[out] netif    The interface. May not be `NULL`.
 * @param[in] stack     The stack for the network interface's thread.
 * @param[in] stacksize Size of @p stack.
 * @param[in] priority  Priority for the network interface's thread.
 * @param[in] name      Name for the network interface. May be NULL.
 * @param[in] dev       Device for the interface.
 * @param[in] ops       Operations for the network interface.
 *
 * @note If @ref DEVELHELP is defined netif_params_t::name is used as the
 *       name of the network interface's thread.
 *
 * @return  0 on success
 * @return  negative number on error
 */
int gnrc_netif_waye_create(gnrc_netif_t *netif,char *stack, int stacksize, char priority, const char *name, netdev_t *dev);

#ifdef __cplusplus
}
#endif

#endif /* NET_GNRC_WAYE_WAYE_H */
/** @} */
