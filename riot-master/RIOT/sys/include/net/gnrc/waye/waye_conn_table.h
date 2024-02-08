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
 * @brief       WAYE connectivity table
 * @internal
 * @author      Raphael Bidaud  <bidaudr@gmail.com>
 */

#ifndef NET_GNRC_WAYE_CONN_TABLE_H
#define NET_GNRC_WAYE_CONN_TABLE_H

#include <stdint.h>
#include <stdbool.h>

#include "byteorder.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief   Sector choice policies
 */
typedef enum {
	SECTOR_POLICY_DOWN,
	SECTOR_POLICY_UP,
	SECTOR_POLICY_MEAN,
	SECTOR_POLICY_BEST_MIN,
	SECTOR_POLICY_BEST_MAX,
} gnrc_waye_conn_table_sector_policy_t;

/**
 * @brief   Information about a node in a sector
 */
typedef struct gnrc_waye_conn_table_sector_info {
	bool found;
	int8_t rssi_waye_min;
	int8_t rssi_waye_max;
	int8_t rssi_hello_min;
	int8_t rssi_hello_max;
	uint8_t lqi_waye_min;
	uint8_t lqi_waye_max;
	uint8_t lqi_hello_min;
	uint8_t lqi_hello_max;
	int8_t calc;
} gnrc_waye_conn_table_sector_info_t;

/**
 * @brief   Entry of the connectivity table
 */
typedef struct gnrc_waye_conn_table_entry {
	le_uint16_t addr;
	uint8_t best_sector;
	uint8_t node_type;
	gnrc_waye_conn_table_sector_info_t sector_info[GNRC_WAYE_SECTORS];	/* Info about the node for each sector */
	struct gnrc_waye_conn_table_entry *next;
} gnrc_waye_conn_table_entry_t;

/**
 * @brief   Clears the connectivity table
 */
void waye_conn_table_clear(void);

/**
 * @brief   Add an entry in the connectivity table when a node
 *          is found in a sector
 *
 * @note    The function allocates a new entry if the node is not in the table.
 *          It updates the min and max RSSI and LQI if an entry already exists
 *          for this node in this sector.
 *
 * @param[in] addr          Address of the node
 * @param[in] sector        Sector of the antenna
 * @param[in] rssi_waye     RSSI for sink to node packet
 * @param[in] rssi_hello    RSSI for node to sink packet
 * @param[in] lqi_waye      LQI for sink to node packet
 * @param[in] lqi_hello     LQI for node to sink packet
 * @param[in] node_type		Node type, either an actuator (0), a captor (1), or both (2)
 *
 * @return  0 on success
 * @return  -ENOMEM if error while allocating memory for a new entry
 */
int waye_conn_table_add(le_uint16_t addr, uint8_t sector, int8_t rssi_waye, int8_t rssi_hello, uint8_t lqi_waye, uint8_t lqi_hello, uint8_t node_type);

/**
 * @brief   Delete a node from the connectivity table
 *
 * @param[in] addr  Address of the node to delete
 *
 * @return  0 on success
 * @return  -1 if the node wasn't found
 */
int waye_conn_table_del(le_uint16_t addr);

/**
 * @brief   Find a node in the connectivity table
 *
 * @param[in] addr  Address of the node
 *
 * @return  Pointer to the corresponding entry if it was found
 * @return  NULL if the node wasn't found
 */
gnrc_waye_conn_table_entry_t *waye_conn_table_find_node(le_uint16_t addr);

/**
 * @brief   Get the number of nodes in the connectivity table
 *
 * @return  Number of nodes in the table
 */
unsigned waye_conn_table_nodes_available(void);

/**
 * @brief   Get the number of nodes in a sector
 *
 * @param[in] sector    Sector of the antenna
 *
 * @return  Number of nodes in this sector
 */
unsigned waye_conn_table_nodes_in_sector(uint8_t sector);

/**
 * @brief   Print the connectivity table per node
 */
void waye_conn_table_print_nodes(void);

/**
 * @brief   Print the connectivity table per sector
 */
void waye_conn_table_print_sectors(void);

/**
 * @brief   Update the best sector for each node based on a
 *          sector choice policy
 *
 * @param[in] policy    Sector choice policy
 */
void waye_conn_table_update_best_sector(gnrc_waye_conn_table_sector_policy_t policy);

/**
 * @brief   Get the connectivity table head
 *
 * @return  Pointer to the first entry in the table
 * @return  NULL if the table is empty
 */
gnrc_waye_conn_table_entry_t *waye_conn_table_get_head(void);

#ifdef __cplusplus
}
#endif

#endif /* NET_GNRC_WAYE_CONN_TABLE_H */
/** @} */
