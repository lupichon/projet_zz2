
#include <stdio.h>
#include "shell.h"
#include "net/gnrc.h"
#include "net/gnrc/netif/ieee802154.h"
#include "net/netdev/ieee802154.h"
#include "net/gnrc/waye/hdr.h"
#include "net/gnrc/waye/waye.h"
#include "at86rf215_internal.h"
#include "at86rf215.h"
#include "at86rf215_registers.h"
#include "ztimer.h"

#define ENABLE_DEBUG    0
#include "debug.h"

#define MAC_HEADER_DRP_SIZE     9U

static int _waye_brouilleur_init(gnrc_netif_t *netif);
static int _send(gnrc_netif_t *netif, gnrc_pktsnip_t *pkt);
static gnrc_pktsnip_t *_recv(gnrc_netif_t *netif);
static void _waye_brouilleur_event_cb(netdev_t *dev, netdev_event_t event);
int brouilleur_send(gnrc_netif_t *netif, gnrc_waye_frame_data_reply_t *data_reply);

static uint8_t header[MAC_HEADER_DRP_SIZE];
static bool do_not_scramble = true;

static const gnrc_netif_ops_t waye_brouilleur_ops = {
    .init = _waye_brouilleur_init,
    .send = _send,
    .recv = _recv,
    .get = gnrc_netif_get_from_netdev,
    .set = gnrc_netif_set_from_netdev,
};

int gnrc_netif_waye_create(gnrc_netif_t *netif,char *stack, int stacksize, char priority, const char *name, netdev_t *dev) {

	return gnrc_netif_create(netif, stack, stacksize, priority, name, dev, &waye_brouilleur_ops);
}

static gnrc_pktsnip_t *_make_netif_hdr(uint8_t *mhr)
{
    gnrc_netif_hdr_t *hdr;
    gnrc_pktsnip_t *snip;
    uint8_t src[IEEE802154_LONG_ADDRESS_LEN], dst[IEEE802154_LONG_ADDRESS_LEN];
    int src_len, dst_len;
    le_uint16_t _pan_tmp;   /* TODO: hand-up PAN IDs to GNRC? */

    dst_len = ieee802154_get_dst(mhr, dst, &_pan_tmp);
    src_len = ieee802154_get_src(mhr, src, &_pan_tmp);
    if ((dst_len < 0) || (src_len < 0)) {
        DEBUG("_make_netif_hdr: unable to get addresses\n");
        return NULL;
    }
    /* allocate space for header */
    snip = gnrc_netif_hdr_build(src, (size_t)src_len, dst, (size_t)dst_len);
    if (snip == NULL) {
        DEBUG("_make_netif_hdr: no space left in packet buffer\n");
        return NULL;
    }
    hdr = snip->data;
    /* set broadcast flag for broadcast destination */
    if ((dst_len == 2) && (dst[0] == 0xff) && (dst[1] == 0xff)) {
        hdr->flags |= GNRC_NETIF_HDR_FLAGS_BROADCAST;
        do_not_scramble = true;
    }
    else {
        do_not_scramble = false;
    }
    /* set flags for pending frames */
    if (mhr[0] & IEEE802154_FCF_FRAME_PEND) {
        hdr->flags |= GNRC_NETIF_HDR_FLAGS_MORE_DATA;
    }
    if(!do_not_scramble){
        for(uint8_t i = 0 ; i < MAC_HEADER_DRP_SIZE ; i++){
            header[i] = mhr[i];
        }
    }
    return snip;
}

static gnrc_pktsnip_t *_recv(gnrc_netif_t *netif)
{
    netdev_t *dev = netif->dev;
    netdev_ieee802154_rx_info_t rx_info;
    gnrc_pktsnip_t *pkt = NULL;
    int bytes_expected = dev->driver->recv(dev, NULL, 0, NULL);

    if (bytes_expected >= (int)IEEE802154_MIN_FRAME_LEN) {
        int nread;

        pkt = gnrc_pktbuf_add(NULL, NULL, bytes_expected, GNRC_NETTYPE_UNDEF);
        if (pkt == NULL) {
            DEBUG("_recv_brouilleur: cannot allocate pktsnip.\n");
            /* Discard packet on netdev device */
            dev->driver->recv(dev, NULL, bytes_expected, NULL);
            return NULL;
        }
        nread = dev->driver->recv(dev, pkt->data, bytes_expected, &rx_info);
        if (nread <= 0) {
            gnrc_pktbuf_release(pkt);
            return NULL;
        }


        if (netif->flags & GNRC_NETIF_FLAGS_RAWMODE) {
            /* Raw mode, skip packet processing, but provide rx_info via
             * GNRC_NETTYPE_NETIF */
            gnrc_pktsnip_t *netif_snip = gnrc_netif_hdr_build(NULL, 0, NULL, 0);
            if (netif_snip == NULL) {
                DEBUG("_recv_ieee802154: no space left in packet buffer\n");
                gnrc_pktbuf_release(pkt);
                return NULL;
            }
            gnrc_netif_hdr_t *hdr = netif_snip->data;
            hdr->lqi = rx_info.lqi;
            hdr->rssi = rx_info.rssi;
            gnrc_netif_hdr_set_netif(hdr, netif);
            pkt = gnrc_pkt_append(pkt, netif_snip);
        }
        else {
            /* Normal mode, try to parse the frame according to IEEE 802.15.4 */
            gnrc_pktsnip_t *ieee802154_hdr, *netif_hdr;
            gnrc_netif_hdr_t *hdr;
            size_t mhr_len = ieee802154_get_frame_hdr_len(pkt->data);
            uint8_t *mhr = pkt->data;
            /* nread was checked for <= 0 before so we can safely cast it to
             * unsigned */
            if ((mhr_len == 0) || ((size_t)nread < mhr_len)) {
                DEBUG("_recv_ieee802154: illegally formatted frame received\n");
                gnrc_pktbuf_release(pkt);
                return NULL;
            }
            netif_hdr = _make_netif_hdr(mhr);
            if (netif_hdr == NULL) {
                DEBUG("_recv_ieee802154: no space left in packet buffer\n");
                gnrc_pktbuf_release(pkt);
                return NULL;
            }
            hdr = netif_hdr->data;

            hdr->lqi = rx_info.lqi;
            hdr->rssi = rx_info.rssi;

            gnrc_netif_hdr_set_netif(hdr, netif);
            dev->driver->get(dev, NETOPT_PROTO, &pkt->type, sizeof(pkt->type));
            if (IS_ACTIVE(ENABLE_DEBUG)) {
                char src_str[GNRC_NETIF_HDR_L2ADDR_PRINT_LEN];

                DEBUG("_recv_ieee802154: received packet from %s of length %u\n",
                    gnrc_netif_addr_to_str(gnrc_netif_hdr_get_src_addr(hdr),
                                            hdr->src_l2addr_len,
                                            src_str),
                    nread);
            }
            /* mark IEEE 802.15.4 header */
            ieee802154_hdr = gnrc_pktbuf_mark(pkt, mhr_len, GNRC_NETTYPE_UNDEF);
            if (ieee802154_hdr == NULL) {
                DEBUG("_recv_ieee802154: no space left in packet buffer\n");
                gnrc_pktbuf_release(pkt);
                gnrc_pktbuf_release(netif_hdr);
                return NULL;
            }
            nread -= ieee802154_hdr->size;
            gnrc_pktbuf_remove_snip(pkt, ieee802154_hdr);
            pkt = gnrc_pkt_append(pkt, netif_hdr);
        }

        DEBUG("_recv_ieee802154: reallocating MAC payload for upper layer.\n");
        gnrc_pktbuf_realloc_data(pkt, nread);
    } else if (bytes_expected > 0) {
        DEBUG("_recv_ieee802154: received frame is too short\n");
        dev->driver->recv(dev, NULL, bytes_expected, NULL);
    }

    return pkt;
}
/* Should only be called to scramble a received DRP with an header size of 9 */
int _send(gnrc_netif_t *netif, gnrc_pktsnip_t *pkt) {
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

    if(!do_not_scramble){
        /* fill MAC header with our header table, everything is hard set */
        dev_pan.u8[0] = header[3];
        dev_pan.u8[1] = header[4];
	    if ((res = ieee802154_set_frame_hdr(mhr, src, src_len,
										dst, dst_len, dev_pan,
										dev_pan, header[0], header[2])) == 0) {
		    DEBUG("_send_ieee802154: Error preperaring frame\n");
		    return -EINVAL;
        }
        for(unsigned i = 0 ; i < MAC_HEADER_DRP_SIZE ; i++) {
            DEBUG("%x ",mhr[i]);
        }
        fflush(stdout);
    } else {
        /* fill MAC header, seq should be set by device */
	    if ((res = ieee802154_set_frame_hdr(mhr, src, src_len,
										dst, dst_len, dev_pan,
										dev_pan, flags, state->seq++)) == 0) {
		DEBUG("_send_ieee802154: Error preperaring frame\n");
		return -EINVAL;
	}
    }
	

	/* prepare iolist for netdev / mac layer */
	iolist_t iolist = { 	
		.iol_next = (iolist_t *)pkt->next,
		.iol_base = mhr,
		.iol_len = (size_t)res
	};

#ifdef MODULE_GNRC_MAC
	if (netif->mac.mac_info & GNRC_NETIF_MAC_INFO_CSMA_ENABLED) {
		DEBUG("CSMACA SEND\n");
		res = csma_sender_csma_ca_send(dev, &iolist, &netif->mac.csma_conf);
	}
	else {
		DEBUG("MAC SEND\n");
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

int brouilleur_send(gnrc_netif_t *netif, gnrc_waye_frame_data_reply_t *data_reply) {
	//DEBUG("Send data reply\n");

	//const uint8_t addr[] = { 0xff, 0xff };
	const uint8_t addr[] = { 0x47, 0x9f };
	uint8_t saddr[] = {header[8],header[7]};
	gnrc_waye_hdr_t reply_hdr;
	gnrc_pktsnip_t *reply_payload;
	gnrc_pktsnip_t *reply_pkt;
	gnrc_pktsnip_t *netif_pkt;

	reply_hdr.type = GNRC_WAYE_FRAMETYPE_DATA_REPLY;


	//DEBUG("saddr[0] = %x saddr[1] = %x addr[0] = %x addr[1] = %x, length saddr : %u length addr : %u\n",saddr[0],saddr[1],addr[0],addr[1],sizeof(saddr),sizeof(addr));

	reply_payload = gnrc_pktbuf_add(NULL, data_reply, sizeof(*data_reply), 1);
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

	int res = netif->ops->send(netif, netif_pkt);
	if (res < 0) {
		return res;
	}

	return 0;
}

static void _waye_brouilleur_event_cb(netdev_t *dev, netdev_event_t event) {
    gnrc_netif_t *netif = (gnrc_netif_t *) dev->context;

    if (event == NETDEV_EVENT_ISR) {

        msg_t msg = { .type = NETDEV_MSG_TYPE_EVENT,
                      .content = { .ptr = netif } };
        if (msg_send(&msg, netif->pid) <= 0) {
            DEBUG("[brouilleur] lost_interrupt\n");
        }
    }
    else {
        DEBUG("gnrc_netif: event triggered -> %i\n", event);
        gnrc_pktsnip_t *pkt = NULL;
        switch (event) {
            case NETDEV_EVENT_RX_COMPLETE:
                pkt = netif->ops->recv(netif);
                uint8_t *ptr = pkt->data;
                if(!do_not_scramble && *ptr == GNRC_WAYE_FRAMETYPE_DATA_REPLY) {
                    gnrc_waye_frame_data_reply_t * frame_data = (gnrc_waye_frame_data_reply_t *) (ptr + 1);
                    //ztimer_sleep(ZTIMER_USEC,3000);
                    brouilleur_send(netif,frame_data);
                }
                gnrc_pktbuf_release(pkt);
                break;

            case NETDEV_EVENT_TX_COMPLETE:
                printf("Frame scrambled !\n");
                break;

            default:
                DEBUG("gnrc_netif: warning: unhandled event %u.\n", event);
        }
    }    
}

static int _waye_brouilleur_init(gnrc_netif_t *netif) {
    netdev_t *dev = netif->dev;
    /* register the event callback with the device driver */
    dev->event_callback = _waye_brouilleur_event_cb;
    dev->context = netif;

    int res = dev->driver->init(dev);
    if (res < 0) {
        return res;
    }

    netopt_enable_t enable = NETOPT_ENABLE;
    dev->driver->set(dev, NETOPT_PROMISCUOUSMODE, &enable, sizeof(enable));

    enable = NETOPT_DISABLE;
	dev->driver->set(dev, NETOPT_CSMA, &enable, sizeof(enable));
    netdev_ieee802154_t *device_state = container_of(dev,netdev_ieee802154_t, netdev);
    at86rf215_t * at86rf215_dev = container_of(device_state, at86rf215_t, netdev);
    for(unsigned i = 0 ; i < 4 ; i ++) {
        DEBUG("ADDR : %x\n",at86rf215_get_addr_short(at86rf215_dev,i));
    }
    uint8_t amcs = at86rf215_reg_read(at86rf215_dev, at86rf215_dev->BBC->RG_AMCS);

    /* disable AACK*/
    amcs &= ~AMCS_AACK_MASK;

    at86rf215_reg_write(at86rf215_dev, at86rf215_dev->BBC->RG_AMCS, amcs);
    return 0;   
}

int main(void) {

	char line_buf[SHELL_DEFAULT_BUFSIZE];
	shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

	return 0;
}