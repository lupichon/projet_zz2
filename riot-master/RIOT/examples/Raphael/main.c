/*
 * Copyright (C) 2014 Freie Universität Berlin
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup     examples
 * @{
 *
 * @file
 * @brief       Hello World application
 *
 * @author      Kaspar Schleiser <kaspar@schleiser.de>
 * @author      Ludwig Knüpfer <ludwig.knuepfer@fu-berlin.de>
 *
 * @}
 */


#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "net/gnrc.h"
#include "net/gnrc/netif/ieee802154.h"
#include "net/netdev/ieee802154.h"
#include "net/gnrc/waye/waye.h"
#include "net/gnrc/waye/waye_conn_table.h"
#include "net/gnrc/waye/types.h"
#include "net/gnrc/netif/mac.h"
//#include "net/gnrc/mac/internal.h"
//#include "net/gnrc/link_layer/waye/include/waye_internal.h"

/******************* DELETE ME ***********/

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
int _gnrc_hello_transmit(gnrc_netif_t *netif, gnrc_pktsnip_t *pkt);

#ifdef __cplusplus
}
#endif

#endif /* WAYE_INTERNAL_H */
/** @} */

/********************** PLEASE **************/



#include "xtimer.h"
#include "random.h"
#include "sba_sp6t.h"
#include "debug.h"
//ENABLE_DEBUG = 1



#include <stdio.h>
#include "thread.h"
#include "xtimer.h"
#include "net/gnrc.h"
#include "net/gnrc/netreg.h"
#include "shell.h"
#include "led.h"
#include "net/gnrc/waye/hdr.h"

#include "net/gnrc.h"
#include "net/gnrc/netif/ieee802154.h"
#include "net/netdev/ieee802154.h"
#include "net/gnrc/waye/waye.h"
#include "net/gnrc/waye/waye_conn_table.h"
#include "net/gnrc/waye/types.h"
#include "net/gnrc/netif/mac.h"
//#include "net/gnrc/mac/internal.h"
#include "random.h"
#include "sba_sp6t.h"


#include "raw_send.h"

#if defined(MODULE_AT86RF233) || defined(MODULE_AT86RF231) || defined(MODULE_AT86RF215)
#include "at86rf2xx_internal.h"
#include "at86rf2xx.h"
#include "at86rf2xx_registers.h"
#endif


#define TOS_AM_GROUP (6)
#define Q_SZ (8)


typedef struct __attribute__((packed)) {
	uint8_t amgroup;
	uint8_t padding;
	uint8_t counter;
} ctr_msg_t;

char recv_thread_stack[THREAD_STACKSIZE_MAIN];
char send_thread_stack[THREAD_STACKSIZE_MAIN];
gnrc_netif_t *netif;
static gnrc_waye_frame_hello_t hello_frame;
static void waye_nodes_hellos(gnrc_netif_t *netif);


int _gnrc_hello_transmit(gnrc_netif_t *netif, gnrc_pktsnip_t *pkt) {
	netdev_t *dev = netif->dev;
	netdev_ieee802154_t *state = (netdev_ieee802154_t *)netif->dev;
	gnrc_netif_hdr_t *netif_hdr;
	const uint8_t *src, *dst = NULL;
	int res = 0;
	size_t src_len, dst_len;
	uint8_t mhr[IEEE802154_MAX_HDR_LEN];
	uint8_t flags = (uint8_t)(state->flags & NETDEV_IEEE802154_SEND_MASK);
	le_uint16_t dev_pan = byteorder_btols(byteorder_htons(state->pan));

	flags |= IEEE802154_FCF_TYPE_DATA;
	if (pkt == NULL) {
		printf("_send_ieee802154: pkt was NULL\n");
		return -EINVAL;
	}
	if (pkt->type != GNRC_NETTYPE_NETIF) {
		printf("_send_ieee802154: first header is not generic netif header\n");
		return -EBADMSG;
	}
	printf("EVERYTHING OK AFTER THE DEBUG\n");
	netif_hdr = pkt->data;
	if (netif_hdr->flags & GNRC_NETIF_HDR_FLAGS_MORE_DATA) {
		/* Set frame pending field */
		flags |= IEEE802154_FCF_FRAME_PEND;
	}
	/* prepare destination address */
	if (netif_hdr->flags & /* If any of these flags is set assume broadcast */
		(GNRC_NETIF_HDR_FLAGS_BROADCAST | GNRC_NETIF_HDR_FLAGS_MULTICAST)) {
		dst = ieee802154_addr_bcast;
		dst_len = IEEE802154_ADDR_BCAST_LEN;
	}
	else {
		dst = gnrc_netif_hdr_get_dst_addr(netif_hdr);
		dst_len = netif_hdr->dst_l2addr_len;
	}
	src_len = netif_hdr->src_l2addr_len;
	if (src_len > 0) {
		src = gnrc_netif_hdr_get_src_addr(netif_hdr);
	}
	else {
		src_len = netif->l2addr_len;
		src = netif->l2addr;
	}
	DEBUG("Sending from : %x %x to : %x %x \n",src[0],src[1],dst[0],dst[1]);
	/* fill MAC header, seq should be set by device */
	if ((res = ieee802154_set_frame_hdr(mhr, src, src_len,
										dst, dst_len, dev_pan,
										dev_pan, flags, state->seq++)) == 0) {
		//DEBUG("_send_ieee802154: Error preperaring frame\n");
		return -EINVAL;
	}

	/* prepare iolist for netdev / mac layer */
	iolist_t iolist = {
		.iol_next = (iolist_t *)pkt->next,
		.iol_base = mhr,
		.iol_len = (size_t)res
	};

#ifdef MODULE_NETSTATS_L2
	if (netif_hdr->flags &
			(GNRC_NETIF_HDR_FLAGS_BROADCAST | GNRC_NETIF_HDR_FLAGS_MULTICAST)) {
		netif->dev->stats.tx_mcast_count++;
	}
	else {
		netif->dev->stats.tx_unicast_count++;
	}
#endif
#ifdef MODULE_GNRC_MAC
	if (netif->mac.mac_info & GNRC_NETIF_MAC_INFO_CSMA_ENABLED) {
		res = csma_sender_csma_ca_send(dev, &iolist, &netif->mac.csma_conf);
	}
	else {
		res = dev->driver->send(dev, &iolist);
	}
#else
	printf("I M GONNA USE DRIVER->SEND\n");
	res = dev->driver->send(dev, &iolist);
	printf("The result is : %d \n",res);
#endif

	/* release old data */
	gnrc_pktbuf_release(pkt);
	return res;
}


int gnrc_waye_send_hellos(gnrc_netif_t *netif, gnrc_waye_frame_hello_t *waye) {
	//DEBUG("Send WAYE\n");
	printf("Everything is fine inside gnrc_waye_send_hellos\n");
	const uint8_t addr[] = { 0xff, 0xff };
	gnrc_waye_hdr_t waye_hdr;
	gnrc_pktsnip_t *waye_payload;
	gnrc_pktsnip_t *waye_pkt;
	gnrc_pktsnip_t *netif_pkt;

	waye_hdr.type = GNRC_WAYE_FRAMETYPE_WAYE;

	waye_payload = gnrc_pktbuf_add(NULL, waye, sizeof(*waye), GNRC_NETTYPE_UNDEF);
	if (waye_payload == NULL) {
		//DEBUG("Error: packet buffer full\n");
		return -ENOBUFS;
	}

	waye_pkt = gnrc_pktbuf_add(waye_payload, &waye_hdr, sizeof(waye_hdr), GNRC_NETTYPE_UNDEF);
	if (waye_pkt == NULL) {
		//DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(waye_payload);
		return -ENOBUFS;
	}

	netif_pkt = gnrc_netif_hdr_build(NULL, 0, addr, sizeof(addr));
	if (netif_pkt == NULL) {
		//DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(waye_pkt);
		return -ENOBUFS;
	}
	netif_pkt->next = waye_pkt;
	printf("everything is fine just before going to _gnrc_hello_transmit\n");
	int res = _gnrc_hello_transmit(netif, netif_pkt);
	if (res < 0) {
		return res;
	}

	return 0;
}

static void waye_nodes_hellos(gnrc_netif_t *netif) {
	//DEBUG("Neighbor discovery\n");

/*
	uint8_t discovery_id;
	uint8_t sector;
	uint8_t round;
	uint8_t selected_slot;
	int8_t rssi_waye;
	uint8_t lqi_waye;
	le_uint16_t addr;
*/

	//hello_frame.discovery_id++;
	hello_frame.sector = 0;
	hello_frame.round = 0;
	//hello_frame.nb_slot = GNRC_WAYE_INITIAL_SLOTS;
	//hello_frame.p = 1.0;
	//hello_frame.slot_size_ms = GNRC_WAYE_ND_SLOT_SIZE_MS;
	printf("waye node hellos is fine !\n");

	gnrc_waye_send_hellos(netif, (gnrc_waye_frame_hello_t *)&hello_frame);
}

void *_handle_incoming_pkt(gnrc_pktsnip_t *pkt) {
	(void) pkt;
	ctr_msg_t *ctr_msg;
	//ctr_msg = (ctr_msg_t *) pkt->data;
	//printf("AM group : %u   padding : %u   counter : %u\n", ctr_msg->amgroup, ctr_msg->padding, ctr_msg->counter);


	if (pkt->size == sizeof(ctr_msg_t)) {

		ctr_msg = (ctr_msg_t *) pkt->data;

		printf("AM group : %u   padding : %u   counter : %u\n", ctr_msg->amgroup, ctr_msg->padding, ctr_msg->counter);

		if (ctr_msg->amgroup == TOS_AM_GROUP) {
			printf("AM group OK\n");
			if ((ctr_msg->counter & 0x01) == 1) {
				printf("LED ON\n");
				LED0_ON;
			} else {
				printf("LED OFF\n");
				LED0_OFF;
			}
		}
	}

	/*
	printf("Paquet recu : %d octets\n", pkt->size);
	uint8_t *ptr = pkt->data;

	for (unsigned i=0; i < pkt->size; i++) {
		printf("%02x ", *ptr);
		ptr++;
	}
	printf("\n");
	*/

	return NULL;
}

void *_recv_thread(void *arg) {
	(void) arg;

	ctr_msg_t message2;
	message2.amgroup = TOS_AM_GROUP;
	message2.counter = 1;

	int ctr = 0;

	static msg_t _msg_q[Q_SZ];
	msg_t msg, reply;
	reply.type = GNRC_NETAPI_MSG_TYPE_ACK;
	reply.content.value = -ENOTSUP;
	msg_init_queue(_msg_q, Q_SZ);
	gnrc_pktsnip_t *pkt = NULL;
	uint8_t *ptr2 = NULL;
	*ptr2=GNRC_WAYE_FRAMETYPE_HELLO;
	pkt->data=ptr2;
	gnrc_netreg_entry_t me_reg = { .demux_ctx = GNRC_NETREG_DEMUX_CTX_ALL, .target.pid = thread_getpid() };
	gnrc_netreg_register(GNRC_NETTYPE_UNDEF, &me_reg);

	while (1) {
		msg_receive(&msg);
		++ctr;
		printf("Message received %d\n", ctr);
		switch (msg.type) {
			case GNRC_NETAPI_MSG_TYPE_RCV:
		//		printf("i'm on handle incoming pkt\n");
		//		pkt = msg.content.ptr;
				//send_frame(pkt, (sizeof(gnrc_waye_hdr_t) + sizeof(gnrc_waye_frame_hello_t)), "10:6e", 0);
				printf("Sending Back the reception counter but just to 10:6e for now ... : %u\n", message2.counter);			
				waye_nodes_hellos(netif);				
				//gnrc_waye_send_waye(netif, (gnrc_waye_frame_waye_t *)&waye_frame);
				//printf("Sending Back the reception counter but just to 10:6e for now ... : %u\n", message2.counter);
				++message2.counter;
				//send_frame(&message2, sizeof(ctr_msg_t), pkt, 0);
		//		_handle_incoming_pkt(pkt); // put me back please
				break;
			case GNRC_NETAPI_MSG_TYPE_SND:
				printf("i'm on handle outgoing pkt\n");
	//			pkt = msg.content.ptr;
				//_handle_outgoing_pkt(pkt);
				break;
			case GNRC_NETAPI_MSG_TYPE_SET:
				printf("i'm on set pkt\n");
				break;
			case GNRC_NETAPI_MSG_TYPE_GET:
				printf("i'm on get pkt\n");
				msg_reply(&msg, &reply);
				break;
			default:
				break;
		}

//	msg_reply(&msg, &reply);
	}
	return NULL;
}

void *_send_thread(void *arg) {
	(void) arg;
	//ctr_msg_t message;

	//message.amgroup = TOS_AM_GROUP;
	//message.counter = 0;

/*	while (1) {
		printf("Sending message\n");
		send_frame(&message, sizeof(ctr_msg_t), "ff:ff", 0);
		// printf("Counter : %u\n", message.counter);
		++message.counter;
		xtimer_sleep(1);
	}
*/	return NULL;
}

int main(void) {
	printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
	printf("This board features a(n) %s MCU.\n", RIOT_MCU);

	thread_create(recv_thread_stack, sizeof(recv_thread_stack), THREAD_PRIORITY_MAIN - 2, THREAD_CREATE_STACKTEST, _recv_thread, NULL, "recv thread");

	thread_create(send_thread_stack, sizeof(send_thread_stack), THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST, _send_thread, NULL, "counter thread");

	char line_buf[SHELL_DEFAULT_BUFSIZE];
	shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

	return 0;
}
