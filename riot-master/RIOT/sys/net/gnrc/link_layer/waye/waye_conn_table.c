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
 *
 * @author      Raphael Bidaud  <bidaudr@gmail.com>
 * @}
 */

#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>

#include "net/gnrc.h"
#include "net/gnrc/netif/ieee802154.h"
#include "net/netdev/ieee802154.h"
#include "net/gnrc/waye/waye.h"
#include "net/gnrc/waye/types.h"
#include "net/gnrc/waye/waye_conn_table.h"

#define ENABLE_DEBUG (1)
#include "debug.h"

/* Connectivity table head */
static gnrc_waye_conn_table_entry_t *conn_table_head = NULL;
static unsigned nodes = 0;

void waye_conn_table_clear(void) {
	gnrc_waye_conn_table_entry_t *next;

	while (conn_table_head != NULL) {
		next = conn_table_head->next;
		free(conn_table_head);
		conn_table_head = next;
	}
	nodes = 0;
}

/* Allocates a new entry for the connectivity table */
static inline gnrc_waye_conn_table_entry_t *new_entry(le_uint16_t addr, uint8_t sector, int8_t rssi_waye, int8_t rssi_hello, uint8_t lqi_waye, uint8_t lqi_hello, gnrc_waye_conn_table_entry_t *next, uint8_t node_type) {
	gnrc_waye_conn_table_entry_t *entry = malloc(sizeof(gnrc_waye_conn_table_entry_t));

	if (entry != NULL) {
		entry->addr = addr;
		entry->next = next;

		for (unsigned i=0; i<GNRC_WAYE_SECTORS; i++) {
			if (i == sector) {
				entry->sector_info[i].found = true;
				entry->sector_info[i].rssi_waye_min = rssi_waye;
				entry->sector_info[i].rssi_waye_max = rssi_waye;
				entry->sector_info[i].rssi_hello_min = rssi_hello;
				entry->sector_info[i].rssi_hello_max = rssi_hello;
				entry->sector_info[i].lqi_waye_min = lqi_waye;
				entry->sector_info[i].lqi_waye_max = lqi_waye;
				entry->sector_info[i].lqi_hello_min = lqi_hello;
				entry->sector_info[i].lqi_hello_max = lqi_hello;
			}
			else {
				entry->sector_info[i].found = false;
			}
		}
		entry->node_type = node_type;
	}

	return entry;
}

/* Updates RSSI and LQI for a given sector */
static inline void update_entry(gnrc_waye_conn_table_entry_t *entry, uint8_t sector, int8_t rssi_waye, int8_t rssi_hello, uint8_t lqi_waye, uint8_t lqi_hello, uint8_t node_type) {
	if (entry->sector_info[sector].found) {
		if (rssi_waye < entry->sector_info[sector].rssi_waye_min) { entry->sector_info[sector].rssi_waye_min = rssi_waye; }
		if (rssi_waye > entry->sector_info[sector].rssi_waye_max) { entry->sector_info[sector].rssi_waye_max = rssi_waye; }
		if (rssi_hello < entry->sector_info[sector].rssi_hello_min) { entry->sector_info[sector].rssi_hello_min = rssi_hello; }
		if (rssi_hello > entry->sector_info[sector].rssi_hello_max) { entry->sector_info[sector].rssi_hello_max = rssi_hello; }
		if (lqi_waye < entry->sector_info[sector].lqi_waye_min) { entry->sector_info[sector].lqi_waye_min = lqi_waye; }
		if (lqi_waye > entry->sector_info[sector].lqi_waye_max) { entry->sector_info[sector].lqi_waye_max = lqi_waye; }
		if (lqi_hello < entry->sector_info[sector].lqi_hello_min) { entry->sector_info[sector].lqi_hello_min = lqi_hello; }
		if (lqi_hello > entry->sector_info[sector].lqi_hello_max) { entry->sector_info[sector].lqi_hello_max = lqi_hello; }
	}
	else {
		entry->sector_info[sector].found = true;
		entry->sector_info[sector].rssi_waye_min = rssi_waye;
		entry->sector_info[sector].rssi_waye_max = rssi_waye;
		entry->sector_info[sector].rssi_hello_min = rssi_hello;
		entry->sector_info[sector].rssi_hello_max = rssi_hello;
		entry->sector_info[sector].lqi_waye_min = lqi_waye;
		entry->sector_info[sector].lqi_waye_max = lqi_waye;
		entry->sector_info[sector].lqi_hello_min = lqi_hello;
		entry->sector_info[sector].lqi_hello_max = lqi_hello;
	}
	entry->node_type = node_type;
}

int waye_conn_table_add(le_uint16_t addr, uint8_t sector, int8_t rssi_waye, int8_t rssi_hello, uint8_t lqi_waye, uint8_t lqi_hello, uint8_t node_type) {
	/* Insert at the beginning if head is NULL or has higher addr */
	if (conn_table_head == NULL || conn_table_head->addr.u16 > addr.u16) {
		gnrc_waye_conn_table_entry_t *new = new_entry(addr, sector, rssi_waye, rssi_hello, lqi_waye, lqi_hello, conn_table_head, node_type);

		if (new == NULL) {
			DEBUG("[WAYE]: Cannot allocate memory for new entry\n");
			return -ENOMEM;
		}

		conn_table_head = new;
		nodes++;
	}
	/* Update existing entry if node is first entry */
	else if (conn_table_head->addr.u16 == addr.u16) {
		update_entry(conn_table_head, sector, rssi_waye, rssi_hello, lqi_waye, lqi_hello, node_type);
	}
	else {
		gnrc_waye_conn_table_entry_t *ptr = conn_table_head;

		while (ptr->next != NULL && ptr->next->addr.u16 < addr.u16) {
			ptr = ptr->next;
		}

		/* Update existing entry for node with address addr */
		if (ptr->next != NULL && ptr->next->addr.u16 == addr.u16) {
			update_entry(ptr->next, sector, rssi_waye, rssi_hello, lqi_waye, lqi_hello, node_type);
		}
		/* Insert new entry */
		else {
			gnrc_waye_conn_table_entry_t *new = new_entry(addr, sector, rssi_waye, rssi_hello, lqi_waye, lqi_hello, ptr->next, node_type);

			if (new == NULL) {
				DEBUG("[WAYE]: Cannot allocate memory for new entry\n");
				return -ENOMEM;
			}

			ptr->next = new;
			nodes++;
		}
	}

	return 0;
}

int waye_conn_table_del(le_uint16_t addr) {
	if (conn_table_head == NULL) {
		return -1;
	}

	if (conn_table_head->addr.u16 == addr.u16) {
		gnrc_waye_conn_table_entry_t *next = conn_table_head->next;
		free(conn_table_head);
		conn_table_head = next;
	}
	else {
		gnrc_waye_conn_table_entry_t *ptr = conn_table_head;

		while (ptr->next != NULL && ptr->next->addr.u16 < addr.u16) {
			ptr = ptr->next;
		}

		if (ptr->next != NULL && ptr->next->addr.u16 == addr.u16) {
			gnrc_waye_conn_table_entry_t *next = ptr->next->next;
			free(ptr->next);
			ptr->next = next;
		}
		else {
			return -1;
		}
	}

	nodes--;
	return 0;
}

gnrc_waye_conn_table_entry_t *waye_conn_table_find_node(le_uint16_t addr) {
	gnrc_waye_conn_table_entry_t *ptr = conn_table_head;

	while (ptr != NULL && ptr->addr.u16 < addr.u16) {
		ptr = ptr->next;
	}

	if (ptr != NULL && ptr->addr.u16 == addr.u16) {
		return ptr;
	}

	return NULL;
}

unsigned waye_conn_table_nodes_available(void) {
	return nodes;
}

unsigned waye_conn_table_nodes_in_sector(uint8_t sector) {
	unsigned nodes = 0;

	for (gnrc_waye_conn_table_entry_t *ptr = conn_table_head; ptr != NULL; ptr = ptr->next) {
		if (ptr->sector_info[sector].found) {
			nodes++;
		}
	}

	return nodes;
}

void waye_conn_table_print_nodes(void) {
	printf("\n\nWAYE connectivity table : %u node(s)\n", waye_conn_table_nodes_available());

	if (conn_table_head) {
		for (gnrc_waye_conn_table_entry_t *ptr = conn_table_head; ptr != NULL; ptr = ptr->next) {
			printf("Node %02x :\trssi_waye_min\trssi_waye_max\trssi_hello_min\trssi_hello_max\tlqi_waye_min\tlqi_waye_max\tlqi_hello_min\tlqi_hello_max\n", ptr->addr.u16);

			for (unsigned sector=0; sector<GNRC_WAYE_SECTORS; sector++) {
				if (ptr->sector_info[sector].found) {
					printf("  Sector %u :\t%d\t\t%d\t\t%d\t\t%d\t\t%u\t\t%u\t\t%u\t\t%u\n", sector, ptr->sector_info[sector].rssi_waye_min, ptr->sector_info[sector].rssi_waye_max, ptr->sector_info[sector].rssi_hello_min, ptr->sector_info[sector].rssi_hello_max, ptr->sector_info[sector].lqi_waye_min, ptr->sector_info[sector].lqi_waye_max, ptr->sector_info[sector].lqi_hello_min, ptr->sector_info[sector].lqi_hello_max);
				}
			}
			printf("\n");
		}
	}
	else {
		printf("Empty\n");
	}
}

void waye_conn_table_print_sectors(void) {
	unsigned nodes;

	printf("\n\nWAYE neighbor discovery table :\n\n");

	for (unsigned i=0; i<GNRC_WAYE_SECTORS; i++) {
		nodes = waye_conn_table_nodes_in_sector(i);
		printf("\nSector %u : %u node(s)\n", i, nodes);

		if (nodes > 0) {
			printf("addr\trssi_waye_min\trssi_waye_max\trssi_hello_min\trssi_hello_max\tlqi_waye_min\tlqi_waye_max\tlqi_hello_min\tlqi_hello_max\n");
			for (gnrc_waye_conn_table_entry_t *ptr = conn_table_head; ptr != NULL; ptr = ptr->next) {
				if (ptr->sector_info[i].found) {
					printf("0x%04x\t%d\t\t%d\t\t%d\t\t%d\t\t%u\t\t%u\t\t%u\t\t%u\n", ptr->addr.u16, ptr->sector_info[i].rssi_waye_min, ptr->sector_info[i].rssi_waye_max, ptr->sector_info[i].rssi_hello_min, ptr->sector_info[i].rssi_hello_max, ptr->sector_info[i].lqi_waye_min, ptr->sector_info[i].lqi_waye_max, ptr->sector_info[i].lqi_hello_min, ptr->sector_info[i].lqi_hello_max);
				}
			}
		}
		else {
			printf("Empty\n");
		}
	}
}

void waye_conn_table_update_best_sector(gnrc_waye_conn_table_sector_policy_t policy) {
	unsigned nodes = waye_conn_table_nodes_available();
	unsigned i;
	int8_t best_rssi[nodes];
	
	for (i=0; i<nodes; i++) {
		best_rssi[i] = -110;
	}

	i = 0;
	switch (policy) {
		case SECTOR_POLICY_DOWN:
			if (conn_table_head) {
				for (gnrc_waye_conn_table_entry_t *ptr = conn_table_head; ptr != NULL; ptr = ptr->next) {
					DEBUG("For node address %02x:\n", ptr->addr.u16);
					for (unsigned sector=0; sector<GNRC_WAYE_SECTORS; sector++) {
						if (ptr->sector_info[sector].found) {
							ptr->sector_info[sector].calc = (ptr->sector_info[sector].rssi_waye_min + ptr->sector_info[sector].rssi_waye_max) / 2;

							if (ptr->sector_info[sector].calc > best_rssi[i]) {
								best_rssi[i] = ptr->sector_info[sector].calc;
								ptr->best_sector = sector;
							}
						}
					}
					i++;
					DEBUG("Sector for the DOWN policy: %u\n\n", ptr->best_sector);
				}
			}
			else {
				DEBUG("Empty\n");
			}
			break;

		case SECTOR_POLICY_UP:
			if (conn_table_head) {
				for (gnrc_waye_conn_table_entry_t *ptr = conn_table_head; ptr != NULL; ptr = ptr->next) {
					DEBUG("For node address %02x:\n", ptr->addr.u16);
					for (unsigned sector=0; sector<GNRC_WAYE_SECTORS; sector++) {
						if (ptr->sector_info[sector].found) {
							ptr->sector_info[sector].calc = (ptr->sector_info[sector].rssi_hello_min + ptr->sector_info[sector].rssi_hello_max) / 2;

							if (ptr->sector_info[sector].calc > best_rssi[i]) {
								best_rssi[i] = ptr->sector_info[sector].calc;
								ptr->best_sector = sector;
							}
						}
					}
					i++;
					DEBUG("Sector for the UP policy: %u\n\n", ptr->best_sector);
				}
			}
			else {
				DEBUG("Empty\n");
			}
			break;

		case SECTOR_POLICY_MEAN:
			if (conn_table_head) {
				for (gnrc_waye_conn_table_entry_t *ptr = conn_table_head; ptr != NULL; ptr = ptr->next) {
					DEBUG("For node address %02x:\n", ptr->addr.u16);
					for (unsigned sector=0; sector<GNRC_WAYE_SECTORS; sector++) {
						if (ptr->sector_info[sector].found) {
							ptr->sector_info[sector].calc = (((ptr->sector_info[sector].rssi_waye_min + ptr->sector_info[sector].rssi_waye_max) / 2) + ((ptr->sector_info[sector].rssi_hello_min + ptr->sector_info[sector].rssi_hello_max) / 2)) / 2;

							if (ptr->sector_info[sector].calc > best_rssi[i]) {
								best_rssi[i] = ptr->sector_info[sector].calc;
								ptr->best_sector = sector;
							}
						}
					}
					i++;
					DEBUG("Sector for the MEAN policy: %u\n\n", ptr->best_sector);
				}
			}
			else {
				DEBUG("Empty\n");
			}
			break;

		case SECTOR_POLICY_BEST_MIN:
			if (conn_table_head) {
				for (gnrc_waye_conn_table_entry_t *ptr = conn_table_head; ptr != NULL; ptr = ptr->next) {
					DEBUG("For node address %02x:\n", ptr->addr.u16);
					for (unsigned sector=0; sector<GNRC_WAYE_SECTORS; sector++) {
						if (ptr->sector_info[sector].found) {
							ptr->sector_info[sector].calc = (ptr->sector_info[sector].rssi_waye_min + ptr->sector_info[sector].rssi_hello_min) / 2;

							if (ptr->sector_info[sector].calc > best_rssi[i]) {
								best_rssi[i] = ptr->sector_info[sector].calc;
								ptr->best_sector = sector;
							}
						}
					}
					i++;
					DEBUG("Sector for the BEST_MIN policy: %u\n\n", ptr->best_sector);
				}
			}
			else {
				DEBUG("Empty\n");
			}
			break;

		case SECTOR_POLICY_BEST_MAX:
			if (conn_table_head) {
				for (gnrc_waye_conn_table_entry_t *ptr = conn_table_head; ptr != NULL; ptr = ptr->next) {
					DEBUG("For node address %02x:\n", ptr->addr.u16);
					for (unsigned sector=0; sector<GNRC_WAYE_SECTORS; sector++) {
						if (ptr->sector_info[sector].found) {
							ptr->sector_info[sector].calc = (ptr->sector_info[sector].rssi_waye_max + ptr->sector_info[sector].rssi_hello_max) / 2;

							if (ptr->sector_info[sector].calc > best_rssi[i]) {
								best_rssi[i] = ptr->sector_info[sector].calc;
								ptr->best_sector = sector;
							}
						}
					}
					i++;
					DEBUG("Sector for the BEST_MAX policy: %u\n\n", ptr->best_sector);
				}
			}
			else {
				DEBUG("Empty\n");
			}
			break;

		default:
			DEBUG("Bad policy choice\n\n");
	}
}

gnrc_waye_conn_table_entry_t *waye_conn_table_get_head(void) {
	return conn_table_head;
}