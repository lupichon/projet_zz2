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
 * @brief       Implementation of WAYE's internal functions.
 *
 * @author      Raphael Bidaud  <bidaudr@gmail.com>
 * @}
 */

#include "net/gnrc.h"
#include "net/gnrc/netif/ieee802154.h"
#include "net/netdev/ieee802154.h"
#include "net/gnrc/waye/waye.h"
#include "net/gnrc/netif/mac.h"
#include "include/waye_internal.h"
#include "ecc/golay2412.h"
#include "ecc/interleaver.h"



#define ENABLE_DEBUG (0)
#include "debug.h"



static bool collect_phase = false;

int _gnrc_waye_transmit(gnrc_netif_t *netif, gnrc_pktsnip_t *pkt) {
	netdev_t *dev = netif->dev;
	netdev_ieee802154_t *state = container_of(dev, netdev_ieee802154_t, netdev);
	gnrc_netif_hdr_t *netif_hdr;
	const uint8_t *src, *dst = NULL;
	int res = 0;
	size_t src_len, dst_len;
	uint8_t mhr[IEEE802154_MAX_HDR_LEN];
	uint8_t flags = (uint8_t)(state->flags & NETDEV_IEEE802154_SEND_MASK);
	le_uint16_t dev_pan = byteorder_btols(byteorder_htons(state->pan));

	flags |= IEEE802154_FCF_TYPE_DATA;
	if (pkt == NULL) {
		DEBUG("_send_ieee802154: pkt was NULL\n");
		return -EINVAL;
	}
	if (pkt->type != GNRC_NETTYPE_NETIF) {
		DEBUG("_send_ieee802154: first header is not generic netif header\n");
		return -EBADMSG;
	}
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
	
	if(collect_phase){
		/* set Ack Requirement bit to 1 */
		
		flags |= IEEE802154_FCF_ACK_REQ;
		
		//DEBUG("Sending from : %x %x to : %x %x \n",src[0],src[1],dst[0],dst[1]);
	}
	
	/* fill MAC header, seq should be set by device */
	if ((res = ieee802154_set_frame_hdr(mhr, src, src_len,
										dst, dst_len, dev_pan,
										dev_pan, flags, state->seq++)) == 0) {
		DEBUG("_send_ieee802154: Error preperaring frame\n");
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
		DEBUG("CSMACA SEND\n");
		res = csma_sender_csma_ca_send(dev, &iolist, &netif->mac.csma_conf);
	}
	else {
		//DEBUG("MAC SEND\n");

		res = dev->driver->send(dev, &iolist);

	}
#else
	DEBUG("NORMAL SEND\n");
	res = dev->driver->send(dev, &iolist);
#endif
	/* release old data */
	gnrc_pktbuf_release(pkt);
	return res;
}

int gnrc_waye_send_waye(gnrc_netif_t *netif, gnrc_waye_frame_waye_t *waye) {
	DEBUG("Send WAYE\n");

	const uint8_t addr[] = { 0xff, 0xff };
	gnrc_waye_hdr_t waye_hdr;
	gnrc_pktsnip_t *waye_payload;
	gnrc_pktsnip_t *waye_pkt;
	gnrc_pktsnip_t *netif_pkt;

	waye_hdr.type = GNRC_WAYE_FRAMETYPE_WAYE;

	waye_payload = gnrc_pktbuf_add(NULL, waye, sizeof(*waye), 1);
	if (waye_payload == NULL) {
		DEBUG("Error: packet buffer full\n");
		return -ENOBUFS;
	}

	waye_pkt = gnrc_pktbuf_add(waye_payload, &waye_hdr, sizeof(waye_hdr), 1);
	if (waye_pkt == NULL) {
		DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(waye_payload);
		return -ENOBUFS;
	}

	netif_pkt = gnrc_netif_hdr_build(NULL, 0, addr, sizeof(addr));
	if (netif_pkt == NULL) {
		DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(waye_pkt);
		return -ENOBUFS;
	}
	netif_pkt->next = waye_pkt;
	
	int res = _gnrc_waye_transmit(netif, netif_pkt);
	if (res < 0) {
		return res;
	}

	return 0;
}

int gnrc_waye_send_hellos(gnrc_netif_t *netif, gnrc_waye_frame_hello_t *waye) {
	//DEBUG("Send HELLOS\n");

	const uint8_t addr[] = { 0xff, 0xff };
	gnrc_waye_hdr_t waye_hdr;
	gnrc_pktsnip_t *waye_payload;
	gnrc_pktsnip_t *waye_pkt;
	gnrc_pktsnip_t *netif_pkt;

	waye_hdr.type = GNRC_WAYE_FRAMETYPE_HELLO;

	waye_payload = gnrc_pktbuf_add(NULL, waye, sizeof(*waye), 1);
	if (waye_payload == NULL) {
		DEBUG("Error: packet buffer full\n");
		return -ENOBUFS;
	}

	waye_pkt = gnrc_pktbuf_add(waye_payload, &waye_hdr, sizeof(waye_hdr), 1);
	if (waye_pkt == NULL) {
		DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(waye_payload);
		return -ENOBUFS;
	}

	netif_pkt = gnrc_netif_hdr_build(NULL, 0, addr, sizeof(addr));
	if (netif_pkt == NULL) {
		DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(waye_pkt);
		return -ENOBUFS;
	}
	netif_pkt->next = waye_pkt;

	int res = _gnrc_waye_transmit(netif, netif_pkt);
	if (res < 0) {
		return res;
	}

	return 0;
}

int gnrc_waye_send_hellos_coop(gnrc_netif_t *netif, gnrc_waye_frame_hello_t *hello, le_uint16_t * addr_list, uint8_t list_size) {
	//DEBUG("Send HELLOS\n");

	const uint8_t addr[] = { 0xff, 0xff };
	gnrc_waye_hdr_t waye_hdr;
	gnrc_pktsnip_t *waye_discovered;
	gnrc_pktsnip_t *waye_payload;
	gnrc_pktsnip_t *waye_pkt;
	gnrc_pktsnip_t *netif_pkt;

	waye_hdr.type = GNRC_WAYE_FRAMETYPE_HELLO;

	waye_discovered = gnrc_pktbuf_add(NULL, addr_list, sizeof(addr_list[0]) * list_size, 1);
	if (waye_discovered == NULL) {
		DEBUG("Error: packet buffer full\n");
		return -ENOBUFS;
	}

	waye_payload = gnrc_pktbuf_add(waye_discovered, hello, sizeof(*hello), 1);
	if (waye_payload == NULL) {
		DEBUG("Error: packet buffer full\n");
		return -ENOBUFS;
	}

	waye_pkt = gnrc_pktbuf_add(waye_payload, &waye_hdr, sizeof(waye_hdr), 1);
	if (waye_pkt == NULL) {
		DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(waye_payload);
		return -ENOBUFS;
	}

	netif_pkt = gnrc_netif_hdr_build(NULL, 0, addr, sizeof(addr));
	if (netif_pkt == NULL) {
		DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(waye_pkt);
		return -ENOBUFS;
	}
	netif_pkt->next = waye_pkt;

	int res = _gnrc_waye_transmit(netif, netif_pkt);
	if (res < 0) {
		return res;
	}

	return 0;
}

int gnrc_waye_send_block_ack(gnrc_netif_t *netif, gnrc_waye_frame_hello_ack_t *hello_ack, le_uint16_t addr_list[], unsigned size) {
	DEBUG("Send block ACK\n");

	const uint8_t addr[] = { 0xff, 0xff };
	gnrc_waye_hdr_t ack_hdr;
	gnrc_pktsnip_t *ack_addr_list;
	gnrc_pktsnip_t *ack_payload;
	gnrc_pktsnip_t *ack_pkt;
	gnrc_pktsnip_t *netif_pkt;

	ack_hdr.type = GNRC_WAYE_FRAMETYPE_HELLO_ACK;

	ack_addr_list = gnrc_pktbuf_add(NULL, addr_list, sizeof(addr_list[0]) * size, 1);
	if (ack_addr_list == NULL) {
		DEBUG("Error: packet buffer full\n");
		return -ENOBUFS;
	}

	ack_payload = gnrc_pktbuf_add(ack_addr_list, hello_ack, sizeof(*hello_ack), 1);
	if (ack_payload == NULL) {
		DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(ack_addr_list);
		return -ENOBUFS;
	}

	ack_pkt = gnrc_pktbuf_add(ack_payload, &ack_hdr, sizeof(ack_hdr), 1);
	if (ack_pkt == NULL) {
		DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(ack_payload);
		return -ENOBUFS;
	}

	netif_pkt = gnrc_netif_hdr_build(NULL, 0, addr, sizeof(addr));
	if (netif_pkt == NULL) {
		DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(ack_pkt);
		return -ENOBUFS;
	}
	netif_pkt->next = ack_pkt;

	int res = _gnrc_waye_transmit(netif, netif_pkt);
	if (res < 0) {
		return res;
	}

	return 0;
}

int gnrc_waye_send_data_poll(gnrc_netif_t *netif, gnrc_waye_frame_data_poll_t *data_poll, le_uint16_t addr_list[], unsigned size) {
	//DEBUG("Send data poll\n");

	const uint8_t addr[] = { 0xff, 0xff };
	gnrc_waye_hdr_t poll_hdr;
	gnrc_pktsnip_t *poll_addr_list;
	gnrc_pktsnip_t *poll_payload;
	gnrc_pktsnip_t *poll_pkt;
	gnrc_pktsnip_t *netif_pkt;

	poll_hdr.type = GNRC_WAYE_FRAMETYPE_DATA_POLL;

	poll_addr_list = gnrc_pktbuf_add(NULL, addr_list, sizeof(addr_list[0]) * size, 1);
	if (poll_addr_list == NULL) {
		DEBUG("Error: packet buffer full\n");
		return -ENOBUFS;
	}

	poll_payload = gnrc_pktbuf_add(poll_addr_list, data_poll, sizeof(*data_poll), 1);
	if (poll_payload == NULL) {
		DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(poll_addr_list);
		return -ENOBUFS;
	}

	poll_pkt = gnrc_pktbuf_add(poll_payload, &poll_hdr, sizeof(poll_hdr), 1);
	if (poll_pkt == NULL) {
		DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(poll_payload);
		return -ENOBUFS;
	}

	netif_pkt = gnrc_netif_hdr_build(NULL, 0, addr, sizeof(addr));
	if (netif_pkt == NULL) {
		DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(poll_pkt);
		return -ENOBUFS;
	}
	netif_pkt->next = poll_pkt;

	int res = _gnrc_waye_transmit(netif, netif_pkt);
	if (res < 0) {
		return res;
	}

	return 0;
}

int gnrc_waye_send_data_reply(gnrc_netif_t *netif, gnrc_waye_frame_data_reply_t *data_reply, uint8_t *sink_addr, uint8_t *node_addr) {
	//DEBUG("Send data reply\n");

	//const uint8_t addr[] = { 0xff, 0xff };
	uint8_t addr[] = { sink_addr[1], sink_addr[0]};
	//addr[1] = 0x49;
	//addr[0] = 0x3b;
	uint8_t saddr[] = {node_addr[0],node_addr[1]};
	gnrc_waye_hdr_t reply_hdr;
	gnrc_pktsnip_t *reply_payload;
	gnrc_pktsnip_t *reply_pkt;
	gnrc_pktsnip_t *netif_pkt;
	size_t data_size = sizeof(*data_reply);
	data_reply->seq_num += 1;
	unsigned char enc_data[data_size*2];
	unsigned char enc_data_interleaver[data_size*2];
	golay2412_encode(data_size,(unsigned char*) data_reply,enc_data);
	interleaver q = interleaver_create(data_size*2);
	interleaver_encode(q,enc_data,enc_data_interleaver);
	interleaver_destroy(q);

	reply_hdr.type = GNRC_WAYE_FRAMETYPE_DATA_REPLY;


	//DEBUG("saddr[0] = %x saddr[1] = %x addr[0] = %x addr[1] = %x, length saddr : %u length addr : %u\n",saddr[0],saddr[1],addr[0],addr[1],sizeof(saddr),sizeof(addr));
/*
	reply_payload = gnrc_pktbuf_add(NULL, data_reply, sizeof(*data_reply), 1);
	if (reply_payload == NULL) {
		DEBUG("Error: packet buffer full\n");
		return -ENOBUFS;
	}
*/
	reply_payload = gnrc_pktbuf_add(NULL, enc_data_interleaver, data_size*2, 1);
	if (reply_payload == NULL) {
		DEBUG("Error: packet buffer full\n");
		return -ENOBUFS;
	}

	reply_pkt = gnrc_pktbuf_add(reply_payload, &reply_hdr, sizeof(reply_hdr), 1);
	if (reply_pkt == NULL) {
		DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(reply_payload);
		return -ENOBUFS;
	}

	netif_pkt = gnrc_netif_hdr_build(saddr, sizeof(saddr), addr, sizeof(addr));
	if (netif_pkt == NULL) {
		DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(reply_pkt);
		return -ENOBUFS;
	}
	netif_pkt->next = reply_pkt;
	collect_phase = true;
	int res = _gnrc_waye_transmit(netif, netif_pkt);
	if (res < 0) {
		return res;
	}

	return 0;
}
/*
int gnrc_waye_send_data_golay(gnrc_netif_t *netif, gnrc_waye_frame_data_golayed_t *data_reply, uint8_t *sink_addr, uint8_t *node_addr, uint8_t crc_state) {

	uint8_t addr[] = { sink_addr[1], sink_addr[0]};
	uint8_t saddr[] = {node_addr[0],node_addr[1]};

	gnrc_waye_hdr_t reply_hdr;
	gnrc_pktsnip_t *reply_payload;
	gnrc_pktsnip_t *reply_pkt;
	gnrc_pktsnip_t *netif_pkt;

	size_t data_size = sizeof(*data_reply);	
	unsigned char data_reply_interleaver[data_size];
	
	interleaver q = interleaver_create(data_size);
	interleaver_set_depth(q,4);
	interleaver_encode(q,(unsigned char*)data_reply,data_reply_interleaver);
	interleaver_destroy(q);
	
	if(crc_state == 0) {
		for( int i = 9 ; i < 18 ; i++){
			data_reply_interleaver[i] ^= 7;
		}
	}


	
	reply_hdr.type = GNRC_WAYE_FRAMETYPE_DATA_REPLY;

	reply_payload = gnrc_pktbuf_add(NULL, data_reply_interleaver, data_size, 1);
	if (reply_payload == NULL) {
		DEBUG("Error: packet buffer full\n");
		return -ENOBUFS;
	}

	reply_pkt = gnrc_pktbuf_add(reply_payload, &reply_hdr, sizeof(reply_hdr), 1);
	if (reply_pkt == NULL) {
		DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(reply_payload);
		return -ENOBUFS;
	}

	netif_pkt = gnrc_netif_hdr_build(saddr, sizeof(saddr), addr, sizeof(addr));
	if (netif_pkt == NULL) {
		DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(reply_pkt);
		return -ENOBUFS;
	}
	netif_pkt->next = reply_pkt;
	collect_phase = true;
	int res = _gnrc_waye_transmit(netif, netif_pkt);
	if (res < 0) {
		return res;
	}

	return 0;


}
*/

int gnrc_waye_send_data_golay(gnrc_netif_t *netif, unsigned char *data_reply, uint8_t *sink_addr, uint8_t *node_addr, uint8_t crc_state, uint8_t data_size) {

	uint8_t addr[] = { sink_addr[1], sink_addr[0]};
	uint8_t saddr[] = {node_addr[0],node_addr[1]};

	gnrc_waye_hdr_t reply_hdr;
	gnrc_pktsnip_t *reply_payload;
	gnrc_pktsnip_t *reply_pkt;
	gnrc_pktsnip_t *netif_pkt;

	//size_t data_size = sizeof(*data_reply);	


	if(crc_state == 0) {
		*(data_reply + 1 ) ^= 0x00;
		/*
		for( int i = 2 ; i < 5 ; i++){
			*(data_reply + i ) ^= 0xa0;
			// *(data_reply + i) <<= 1;
		}
		*/
		// *(data_reply + 9) ^= 0x07;
		
	}

		
	reply_hdr.type = GNRC_WAYE_FRAMETYPE_DATA_REPLY;

	reply_payload = gnrc_pktbuf_add(NULL, data_reply, data_size, 1);
	if (reply_payload == NULL) {
		DEBUG("Error: packet buffer full\n");
		return -ENOBUFS;
	}

	reply_pkt = gnrc_pktbuf_add(reply_payload, &reply_hdr, sizeof(reply_hdr), 1);
	if (reply_pkt == NULL) {
		DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(reply_payload);
		return -ENOBUFS;
	}

	netif_pkt = gnrc_netif_hdr_build(saddr, sizeof(saddr), addr, sizeof(addr));
	if (netif_pkt == NULL) {
		DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(reply_pkt);
		return -ENOBUFS;
	}
	netif_pkt->next = reply_pkt;
	collect_phase = true;
	int res = _gnrc_waye_transmit(netif, netif_pkt);
	if (res < 0) {
		return res;
	}
	

	return 0;


}

int gnrc_waye_send_bitmap_ack(gnrc_netif_t *netif, gnrc_waye_frame_data_ack_t *data_ack, uint8_t bitmap[], unsigned bitmap_size) {
	DEBUG("Send bitmap ACK\n");

	const uint8_t addr[] = { 0xff, 0xff };
	gnrc_waye_hdr_t ack_hdr;
	gnrc_pktsnip_t *ack_bitmap;
	gnrc_pktsnip_t *ack_payload;
	gnrc_pktsnip_t *ack_pkt;
	gnrc_pktsnip_t *netif_pkt;

	ack_hdr.type = GNRC_WAYE_FRAMETYPE_DATA_ACK;

	ack_bitmap = gnrc_pktbuf_add(NULL, bitmap, bitmap_size, 1);
	if (ack_bitmap == NULL) {
		DEBUG("Error: packet buffer full\n");
		return -ENOBUFS;
	}

	ack_payload = gnrc_pktbuf_add(ack_bitmap, data_ack, sizeof(*data_ack), 1);
	if (ack_payload == NULL) {
		DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(ack_bitmap);
		return -ENOBUFS;
	}

	ack_pkt = gnrc_pktbuf_add(ack_payload, &ack_hdr, sizeof(ack_hdr), 1);
	if (ack_pkt == NULL) {
		DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(ack_payload);
		return -ENOBUFS;
	}

	netif_pkt = gnrc_netif_hdr_build(NULL, 0, addr, sizeof(addr));
	if (netif_pkt == NULL) {
		DEBUG("Error: packet buffer full\n");
		gnrc_pktbuf_release(ack_pkt);
		return -ENOBUFS;
	}
	netif_pkt->next = ack_pkt;

	int res = _gnrc_waye_transmit(netif, netif_pkt);
	if (res < 0) {
		return res;
	}

	return 0;
}
