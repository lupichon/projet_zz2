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
 * @brief       WAYE's internal functions.
 * @internal
 * @author      Raphael Bidaud  <bidaudr@gmail.com>
 */

#ifndef WAYE_INTERNAL_H
#define WAYE_INTERNAL_H

#include "net/gnrc/netif.h"
#include "net/gnrc/waye/hdr.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Send a WAYE neighbor discovery message
 *
 * @pre `netif != NULL && waye != NULL`
 *
 * @param[in] netif		ptr to the network interface
 * @param[in] waye		ptr to the waye message structure to send
 *
 * @return	0 on success
 * @return	-ENOBUFS, if no space in packet buffer
 * @return	Any negative error code reported by _gnrc_waye_transmit
 */

int gnrc_waye_send_hellos(gnrc_netif_t *netif, gnrc_waye_frame_hello_t *waye);


int gnrc_waye_send_hellos_coop(gnrc_netif_t *netif, gnrc_waye_frame_hello_t *hello, le_uint16_t * addr_list, uint8_t list_size);


int gnrc_waye_send_waye(gnrc_netif_t *netif, gnrc_waye_frame_waye_t *waye);

/**
 * @brief Send a HELLO ACK message containing a list of addresses to acknowledge
 *
 * @pre `netif != NULL && hello_ack != NULL && addr_list != NULL && size >= 1`
 *
 * @param[in] netif			ptr to the network interface
 * @param[in] hello_ack		ptr to the hello ack message structure to send
 * @param[in] addr_list		array of addresses to acknowledge
 * @param[in] size			number of addresses in the array
 *
 * @return	0 on success
 * @return	-ENOBUFS, if no space in packet buffer
 * @return	Any negative error code reported by _gnrc_waye_transmit
 */
int gnrc_waye_send_block_ack(gnrc_netif_t *netif, gnrc_waye_frame_hello_ack_t *hello_ack, le_uint16_t addr_list[], unsigned size);

/**
 * @brief Send a DATA POLL message containing a list of addresses to poll
 *
 * @pre `netif != NULL && addr_list != NULL && size >= 1`
 *
 * @param[in] netif			ptr to the network interface
 * @param[in] data_poll		ptr to data poll message structure to send
 * @param[in] addr_list		array of addresses to poll
 * @param[in] size			number of addresses in the array
 *
 * @return	0 on success
 * @return	-ENOBUFS, if no space in packet buffer
 * @return	Any negative error code reported by _gnrc_waye_transmit
 */
int gnrc_waye_send_data_poll(gnrc_netif_t *netif, gnrc_waye_frame_data_poll_t *data_poll, le_uint16_t addr_list[], unsigned size);

int gnrc_waye_send_bitmap_ack(gnrc_netif_t *netif, gnrc_waye_frame_data_ack_t *data_ack, uint8_t bitmap[], unsigned bitmap_size);

int gnrc_waye_send_data_reply(gnrc_netif_t *netif, gnrc_waye_frame_data_reply_t *data_reply, uint8_t *sink_addr, uint8_t *node_addr);

//int gnrc_waye_send_data_golay(gnrc_netif_t *netif, gnrc_waye_frame_data_golayed_t *data_reply, uint8_t *sink_addr, uint8_t *node_addr, uint8_t crc_state);

int gnrc_waye_send_data_golay(gnrc_netif_t *netif, unsigned char *data_reply, uint8_t *sink_addr, uint8_t *node_addr, uint8_t crc_state, uint8_t data_size);

/**
 * @brief Send a @ref net_gnrc_pkt "packet" over the network interface in WAYE
 *
 * @internal
 *
 * @pre `netif != NULL && pkt != NULL`
 *
 * @note The function re-formats the content of @p pkt to a format expected
 *       by the netdev_driver_t::send() method of gnrc_netif_t::dev and
 *       releases the packet before returning (so no additional release
 *       should be required after calling this method).
 *
 * @param[in] netif The network interface.
 * @param[in] pkt   A packet to send.
 *
 * @return  The number of bytes actually sent on success
 * @return  -EBADMSG, if the @ref net_gnrc_netif_hdr in @p pkt is missing
 *          or is in an unexpected format.
 * @return  -ENOTSUP, if sending @p pkt in the given format isn't supported
 *          (e.g. empty payload with Ethernet).
 * @return  Any negative error code reported by gnrc_netif_t::dev.
 */
int _gnrc_waye_transmit(gnrc_netif_t *netif, gnrc_pktsnip_t *pkt);

#ifdef __cplusplus
}
#endif

#endif /* WAYE_INTERNAL_H */
/** @} */
