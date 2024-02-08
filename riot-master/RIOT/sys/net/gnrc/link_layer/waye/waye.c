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
 * @brief       Implementation of WAYE
 *
 * @author      Raphael Bidaud  <bidaudr@gmail.com>
 * @author      Elise Decaesteker  <elise.decaesteker@protonmail.com>
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
#include "net/gnrc/mac/internal.h"
#include "include/waye_internal.h"
#include "periph/rtt.h"
#include "xtimer.h"
#include "ztimer.h"
#include "random.h"
#include "sba_sp6t.h"
#include "ecc/golay2412.h"
#include "ecc/interleaver.h"

#if defined(MODULE_AT86RF233) || defined(MODULE_AT86RF231)
#include "at86rf2xx_internal.h"
#include "at86rf2xx.h"
#include "at86rf2xx_registers.h"
#endif

#ifdef MODULE_GNRC_IPV6
#include "net/ipv6/hdr.h"
#endif

#define ENABLE_DEBUG 0
#define ENABLE_DEBUG2 1

#include "debug.h"

#if defined(MODULE_OD) && ENABLE_DEBUG
#include "od.h"
#endif

/* Global variable used to stop the RSSI loop when an interrupt happens */
volatile bool waye_continue_rssi_loop = false;

/* Changed to false at the end of a round by the RTT callback */
static volatile bool waiting_hellos = false;

/* Variable used to end the collision detection at the end of a round */
volatile bool waye_cca_loop = false;

/* Global variable used to end the ED loop */
volatile bool waye_ed_loop = false;

/**
 * @brief WAYE thread's PID
 */
static kernel_pid_t waye_pid;

/**
 * @brief Switched-Beam Antenna device
 */
static sba_sp6t_t sba;

/**
 * @brief Global variable Golay error corrected
 */
volatile bool waye_golay_error_corrected = true;

/**
 * @brief Global variable used by netdev for GOLAY
 */
volatile bool waye_global_collection_phase = false;

volatile uint16_t compteur = 0;
volatile uint16_t compteur2 = 0;

static bool collection_phase = false;
static bool first_discovery = false;
static bool nb_discovery_phase = false;
static bool is_sent_ack = false;
// static bool each_round_first = false;

le_uint16_t ack_addr_list[GNRC_WAYE_SECTORS][GNRC_WAYE_ACK_ADDR_LIST_SIZE];
unsigned ack_addr_list_size[GNRC_WAYE_SECTORS];
static gnrc_waye_frame_waye_t waye_frame;
static gnrc_waye_frame_data_poll_t data_poll;
static gnrc_waye_frame_data_ack_t data_ack;
static netdev_event_t waye_tx_res;
static uint32_t waye_tx_end_counter;
static uint32_t poll_tx_end_counter;
static uint32_t last_rx_start_counter;
static uint32_t rx_start_counters[GNRC_WAYE_RX_START_QUEUE_SIZE];
static unsigned rx_start_num;
static gnrc_waye_hello_counters_t hellos[GNRC_WAYE_HELLO_QUEUE_SIZE];
static gnrc_waye_hello_counters_t non_hellos[GNRC_WAYE_HELLO_QUEUE_SIZE];
static gnrc_waye_data_counters_t datas[GNRC_WAYE_DATA_QUEUE_SIZE];
static uint8_t datas_num;
static unsigned hellos_num;
static unsigned non_hellos_num;
static uint32_t medium_busy_counters[GNRC_WAYE_BUSY_QUEUE_SIZE];
static unsigned medium_busy_num;
static le_uint16_t collect_list[GNRC_WAYE_SECTORS][10];
static unsigned collect_list_size[GNRC_WAYE_SECTORS];
static le_uint16_t actuator_list[GNRC_WAYE_SECTORS][10];
static unsigned actuator_list_size[GNRC_WAYE_SECTORS];
static unsigned collect_sector;
static uint8_t gack_bitmap[GNRC_WAYE_GACK_BITMAP_MAX_SIZE];
static uint8_t gack_bitmap_size;

static int8_t rssi[GNRC_WAYE_RSSI_NUMBER];
static int rssi_avg;
static unsigned pos = 0;

// static gnrc_waye_slot_activity_t slot_activity[70][GNRC_WAYE_INITIAL_SLOTS];
//static uint32_t rssi_time[GNRC_WAYE_RSSI_TABLES_SIZE];
//static int rssi_ed[GNRC_WAYE_RSSI_TABLES_SIZE];
// static uint32_t rssi_fill_time[300];
static uint16_t nround = 0;
static uint8_t nbed;
static uint32_t nbrssi = 0;
static uint32_t nbrssidif = 0;
static uint8_t fill_counter = 0;

static uint16_t nbcollect = 0;
static uint8_t sink_addr[2];
static uint8_t wcrl_in_waye = 0;

static uint8_t numero_de_sequence[12];

static uint32_t temps_debut_collecte = 0;
static uint32_t temps_fin_collecte = 0;
static uint32_t temps_collecte[6];
static bool crcs[12];
static uint16_t total_lost = 0;
static uint8_t fcf_mhr[12];
static uint8_t pktlen = 0, mhrlen = 0, paylen = 0;
// static uint8_t waye_slot = 1;

static gnrc_waye_waiting_node_t * emergency_queue = NULL;
static uint8_t emergency_queue_size = 0;
static uint16_t nb_event = 0;

static gnrc_waye_waiting_node_t * standard_queue[GNRC_WAYE_SECTORS];
static uint8_t standard_queue_size[GNRC_WAYE_SECTORS];

static bool sent_standard = false;

static bool sent_emergency = false;
static gnrc_waye_frame_data_mixed_poll_t data_mixed_poll;

static uint16_t order_id = 0;

char queue_generator_thread_stack[THREAD_STACKSIZE_MAIN];
// static gnrc_waye_waiting_node_t collect_queue[GNRC_WAYE_WAITING_QUEUE_SIZE];
// static gnrc_waye_waiting_node_t * flow_queue = NULL;

//static gnrc_waye_waiting_node_t * waiting_node = NULL;

static unsigned char matrix[24][24];
static uint8_t nb_rx_start = 0;
static unsigned char original_frame[72];
static uint8_t original_pay_len = 0;
static uint8_t nb_ack = 0;

static void waye_init_cb(void *arg);

static int _waye_init(gnrc_netif_t *netif);
static int _send(gnrc_netif_t *netif, gnrc_pktsnip_t *pkt);
static gnrc_pktsnip_t *_recv(gnrc_netif_t *netif);
static void _waye_msg_handler(gnrc_netif_t *netif, msg_t *msg);
static void _waye_event_cb(netdev_t *dev, netdev_event_t event);
static void waye_round_end_cb(void *arg);
static void waye_round_end_handler(void *arg);
static void at86rf2xx_wait_medium_busy(at86rf2xx_t *dev);
static inline int waye_estimate_remaining(unsigned nb_slot_empty, unsigned nb_slot_success, unsigned nb_slot_collision);
static void waye_collect_init(void);
static void waye_collect(gnrc_netif_t *netif);
static void waye_collect_end_cb(void *arg);
static void waye_collect_end_handler(void *arg);
static inline int data_reply_slot(uint32_t poll_tx_end_ticks, uint32_t rx_start_ticks);

static inline void set_bit(uint8_t bitmap[], unsigned bit);
static inline void clear_bit(uint8_t bitmap[], unsigned bit);
static inline int test_bit(uint8_t bitmap[], unsigned bit);

/**
 * @brief If node is found in the connectivity table, randomly adds a poll or actuator order
 * to the FIFO queue passed in the params
 * 
 * @param[in] node 	Address of the node we want to assign a poll or order
 * @param[in] queue	Address of the queue for which we'll be adding an element
 * @param[in] queue_size Number of element in the passed queue before adding an element
 * 
 * @return -1 if node is not found in the connectivity table
 * @return -2 if malloc failed
 * @return Number of element in the passed queue after adding an element
*/
static int8_t add_queue_element(le_uint16_t node, gnrc_waye_waiting_node_t ** queue, uint8_t queue_size);

static void * _thread_add_element_to_queue(void *arg);

static void matrix_interleaver_decode(unsigned char enc_frame[], unsigned char dec_frame[], uint8_t nb_column);
void printu8(uint8_t d);

// A supprimer après utilisation 
static void matrix_linearization(unsigned char * line_matrix, uint8_t nb_column);
static void put_error_in_frame(unsigned char * payload1, uint8_t pay_len1, float error_rate, uint8_t burst_size);

//structure permettant de definir des fonctions specifiques à une interface réseau
static const gnrc_netif_ops_t waye_ops = {
	.init = _waye_init, 					//initialisation du réseau	
	.send = _send,							//envoie de paquets ou de trames
	.recv = _recv,							//recevoir des données
	.get = gnrc_netif_get_from_netdev,		//obtenir des informations sur l'état actuel du réseau
	.set = gnrc_netif_set_from_netdev,		//definir des informations sur le réseau 
	.msg_handler = _waye_msg_handler,		//gestion des évènements sur le réseau 
};

//création de l'interface réseau
int gnrc_netif_waye_create(gnrc_netif_t *netif, char *stack, int stacksize, char priority, const char *name, netdev_t *dev)
{
	return gnrc_netif_create(netif, stack, stacksize, priority, name, dev, &waye_ops);
}

//Création d'un en-tête de paquet réseau à partir d'un en-tête de de trame IEE 802.15.4
static gnrc_pktsnip_t *_make_netif_hdr(uint8_t *mhr)
{
	gnrc_pktsnip_t *snip;
	uint8_t src[IEEE802154_LONG_ADDRESS_LEN], dst[IEEE802154_LONG_ADDRESS_LEN];
	int src_len, dst_len;
	le_uint16_t _pan_tmp;

	dst_len = ieee802154_get_dst(mhr, dst, &_pan_tmp);
	src_len = ieee802154_get_src(mhr, src, &_pan_tmp);
	if ((dst_len < 0) || (src_len < 0))
	{
		DEBUG("_make_netif_hdr: unable to get addresses\n");
		return NULL;
	}
	/* allocate space for header */
	snip = gnrc_netif_hdr_build(src, (size_t)src_len, dst, (size_t)dst_len);
	if (snip == NULL)
	{
		DEBUG("_make_netif_hdr: no space left in packet buffer\n");
		return NULL;
	}
	/* set broadcast flag for broadcast destination */
	if ((dst_len == 2) && (dst[0] == 0xff) && (dst[1] == 0xff))
	{
		gnrc_netif_hdr_t *hdr = snip->data;
		hdr->flags |= GNRC_NETIF_HDR_FLAGS_BROADCAST;
	}
	return snip;
}

//Reception des données 
static gnrc_pktsnip_t *_recv(gnrc_netif_t *netif)
{
	netdev_t *dev = netif->dev;
	netdev_ieee802154_rx_info_t rx_info;
	gnrc_pktsnip_t *pkt = NULL;
	int bytes_expected = dev->driver->recv(dev, NULL, 0, NULL);

	if (bytes_expected >= (int)IEEE802154_MIN_FRAME_LEN)
	{
		int nread;

		pkt = gnrc_pktbuf_add(NULL, NULL, bytes_expected, GNRC_NETTYPE_UNDEF);
		if (pkt == NULL)
		{
			DEBUG("_recv_ieee802154: cannot allocate pktsnip.\n");
			/* Discard packet on netdev device */
			dev->driver->recv(dev, NULL, bytes_expected, NULL);
			return NULL;
		}
		nread = dev->driver->recv(dev, pkt->data, bytes_expected, &rx_info);

		pktlen = nread;

		if (nread <= 0)
		{
			gnrc_pktbuf_release(pkt);
			return NULL;
		}
		if (netif->flags & GNRC_NETIF_FLAGS_RAWMODE)
		{
			/* Raw mode, skip packet processing, but provide rx_info via
			 * GNRC_NETTYPE_NETIF */
			gnrc_pktsnip_t *netif_snip = gnrc_netif_hdr_build(NULL, 0, NULL, 0);
			if (netif_snip == NULL)
			{
				DEBUG("_recv_ieee802154: no space left in packet buffer\n");
				gnrc_pktbuf_release(pkt);
				return NULL;
			}
			gnrc_netif_hdr_t *hdr = netif_snip->data;
			hdr->lqi = rx_info.lqi;
			hdr->rssi = rx_info.rssi;
			hdr->if_pid = netif->pid;
			LL_APPEND(pkt, netif_snip);
		}
		else
		{
			/* Normal mode, try to parse the frame according to IEEE 802.15.4 */
			gnrc_pktsnip_t *ieee802154_hdr, *netif_hdr;
			gnrc_netif_hdr_t *hdr;

			size_t mhr_len = ieee802154_get_frame_hdr_len(pkt->data);

			mhrlen = mhr_len;

			if (mhr_len == 0)
			{
				DEBUG("_recv_ieee802154: illegally formatted frame received\n");
				gnrc_pktbuf_release(pkt);
				return NULL;
			}
			nread -= mhr_len;
			/* mark IEEE 802.15.4 header */
			ieee802154_hdr = gnrc_pktbuf_mark(pkt, mhr_len, GNRC_NETTYPE_UNDEF);
			if (ieee802154_hdr == NULL)
			{
				DEBUG("_recv_ieee802154: no space left in packet buffer\n");
				gnrc_pktbuf_release(pkt);
				return NULL;
			}
			netif_hdr = _make_netif_hdr(ieee802154_hdr->data);
			if (netif_hdr == NULL)
			{
				DEBUG("_recv_ieee802154: no space left in packet buffer\n");
				gnrc_pktbuf_release(pkt);
				return NULL;
			}

			hdr = netif_hdr->data;

#ifdef MODULE_L2FILTER
			if (!l2filter_pass(dev->filter, gnrc_netif_hdr_get_src_addr(hdr),
							   hdr->src_l2addr_len))
			{
				gnrc_pktbuf_release(pkt);
				gnrc_pktbuf_release(netif_hdr);
				DEBUG("_recv_ieee802154: packet dropped by l2filter\n");
				return NULL;
			}
#endif

			hdr->lqi = rx_info.lqi;
			hdr->rssi = rx_info.rssi;
			hdr->if_pid = thread_getpid();
			dev->driver->get(dev, NETOPT_PROTO, &pkt->type, sizeof(pkt->type));
			// DEBUG("_recv_ieee802154: received packet from %s of length %u\n", gnrc_netif_addr_to_str(gnrc_netif_hdr_get_src_addr(hdr), hdr->src_l2addr_len, src_str), nread);
			gnrc_pktbuf_remove_snip(pkt, ieee802154_hdr);
			LL_APPEND(pkt, netif_hdr);
		}

		DEBUG("_recv_ieee802154: reallocating.\n");
		gnrc_pktbuf_realloc_data(pkt, nread);
	}
	else if (bytes_expected > 0)
	{
		DEBUG("_recv_ieee802154: received frame is too short\n");
		dev->driver->recv(dev, NULL, bytes_expected, NULL);
	}

	return pkt;
}

//Envoie de données 
static int _send(gnrc_netif_t *netif, gnrc_pktsnip_t *pkt)
{
	return _gnrc_waye_transmit(netif, pkt);
}

//Calcul de la puissance du signal ()
static inline int8_t at86rf2xx_get_rssi(at86rf2xx_t *dev)
{
	int8_t rssi;
	uint8_t reg;

	reg = at86rf2xx_reg_read(dev, AT86RF2XX_REG__PHY_RSSI);
	reg &= AT86RF2XX_PHY_RSSI_MASK__RSSI;
	rssi = RSSI_BASE_VAL + 3 * reg;

	return rssi;
}

/**
 * @brief   Function called by the device driver on device events
 *
 * @param[in] event     type of event
 */
static void _waye_event_cb(netdev_t *dev, netdev_event_t event)
{
	gnrc_netif_t *netif = (gnrc_netif_t *)dev->context;

	if (event == NETDEV_EVENT_ISR)
	{
		msg_t msg = {.type = NETDEV_MSG_TYPE_EVENT,
					 .content = {.ptr = netif}};
		if (msg_send(&msg, netif->pid) <= 0)
		{
			DEBUG("WARNING: [WAYE] gnrc_netdev: possibly lost interrupt.\n");
		}
	}
	else
	{
		DEBUG("gnrc_netdev: event triggered -> %i\n", event);

		switch (event)
		{
		case NETDEV_EVENT_RX_STARTED:
			DEBUG("[WAYE] NETDEV_EVENT_RX_STARTED\n");
			last_rx_start_counter = rtt_get_counter();
			nb_rx_start++;
			if (nb_discovery_phase && waiting_hellos)
			{
				if (rx_start_num < GNRC_WAYE_RX_START_QUEUE_SIZE)
				{
					rx_start_counters[rx_start_num] = last_rx_start_counter;
					rx_start_num++;
				}
				else
				{
					DEBUG("WARNING [WAYE]: Failed RX_START queue full !\n");
				}
			}
			gnrc_netif_set_rx_started(netif, true);

			if (nb_discovery_phase && waiting_hellos)
			{
				at86rf2xx_wait_medium_busy((at86rf2xx_t *)dev);
			}

			break;

		case NETDEV_EVENT_RX_COMPLETE:
			DEBUG("[WAYE] NETDEV_EVENT_RX_COMPLETE\n");
			uint32_t counter = rtt_get_counter();

			if (collection_phase)
			{

				waye_golay_error_corrected = true;

/*
				gnrc_pktsnip_t *pkt = netif->ops->recv(netif);
				
				uint8_t *ptr = pkt->data;
				paylen = pktlen - mhrlen;
				unsigned char payload[paylen];
				for(int i =  0 ; i < paylen ; i ++){
					payload[i] = *(ptr + i + 1);
				}
				netdev_ieee802154_t *device_state = container_of(dev, netdev_ieee802154_t, netdev);
				at86rf2xx_t *at86rf2xx_dev = container_of(device_state, at86rf2xx_t, netdev);
				// DEBUG2("[WAYE] NETDEV_EVENT_RX_COMPLETE\n");
*/
				uint8_t phr;
				uint8_t mhr[10];
				size_t pkt_len, mhr_len, pay_len;

				
				netdev_ieee802154_t *device_state = container_of(dev, netdev_ieee802154_t, netdev);
				at86rf2xx_t *at86rf2xx_dev = container_of(device_state, at86rf2xx_t, netdev);

				at86rf2xx_set_state(at86rf2xx_dev, AT86RF2XX_STATE_PLL_ON);
				at86rf2xx_fb_start(at86rf2xx_dev);
				at86rf2xx_fb_read(at86rf2xx_dev, &phr, 1);
				pkt_len = (phr & 0x7f) - 2;

				// MHR taken on 10 bytes due to the presence of the frame type in mhr[9]

				at86rf2xx_fb_read(at86rf2xx_dev, mhr, 10);

				mhr_len = ieee802154_get_frame_hdr_len(mhr);
				pay_len = pkt_len - mhr_len - 1;
				uint8_t payload[pay_len];
				at86rf2xx_fb_read(at86rf2xx_dev, payload, pay_len);

				// Ignore FCS
				uint8_t tmp[2];
				at86rf2xx_fb_read(at86rf2xx_dev, tmp, 2);
				(void)tmp;

				at86rf2xx_fb_stop(at86rf2xx_dev);
				at86rf2xx_set_state(at86rf2xx_dev, at86rf2xx_dev->idle_state);

				pktlen = pkt_len;
				mhrlen = mhr_len;
				paylen = pay_len;
				if (mhr[9] == GNRC_WAYE_FRAMETYPE_DATA_REPLY)
				//if (*ptr == GNRC_WAYE_FRAMETYPE_DATA_REPLY)
				{
					// DEBUG2("pkt : %u | mhr : %u | pay : %u\n",pkt_len,mhr_len,pay_len);
					//puts("");
					unsigned char dec_payload[paylen / 2];
					unsigned char enc_payload[paylen];

					memset(enc_payload, 0, paylen);

					matrix_interleaver_decode(payload, enc_payload, paylen / 3);

					golay2412_decode(paylen / 2, enc_payload, dec_payload);

					uint8_t phy_status = at86rf2xx_reg_read(at86rf2xx_dev, AT86RF2XX_REG__PHY_RSSI);
					bool crc_ok = phy_status & AT86RF2XX_PHY_RSSI_MASK__RX_CRC_VALID;

					if (!waye_golay_error_corrected)
					{
						total_lost++;
					}
					if (waye_golay_error_corrected && !crc_ok)
					{
						//uint8_t addr_ack[2] = {dec_payload[1], dec_payload[2]};
						uint8_t addr_ack[2] = {mhr[8], mhr[7]};
						data_ack.poll_id = 139;
						is_sent_ack = true;
						gnrc_waye_send_collect_ack(netif, (gnrc_waye_frame_data_ack_t *)&data_ack, addr_ack);
						nb_ack++;

					}
					if (sent_emergency)
					{
						if (waye_golay_error_corrected)
						{
							DEBUG2("Reduce emergency queue POLL\n");
							gnrc_waye_waiting_node_t * free_memory_pointer = emergency_queue;
							emergency_queue = emergency_queue->next;
							free(free_memory_pointer);
							emergency_queue_size--;
						}
					}

					if(!crc_ok){
						memset(original_frame,0x0f,paylen/2);
						original_pay_len = 0;
						for(int i = 0 ; i < paylen/2 ; i++){
							original_frame[i] ^= dec_payload[i];
							original_pay_len++;
						}
					}
					gnrc_waye_frame_data_reply_t *reply = (gnrc_waye_frame_data_reply_t *) dec_payload;
					memcpy(&datas[datas_num].pkt,reply,sizeof(gnrc_waye_frame_data_reply_t));
					//numero_de_sequence[datas_num] = dec_payload[0];
					numero_de_sequence[datas_num] = mhr[2];

					int slot = data_reply_slot(poll_tx_end_counter, last_rx_start_counter);

					datas[datas_num].slot = slot;
					datas[datas_num].poll_id = dec_payload[4];
					crcs[datas_num] = crc_ok;

					temps_collecte[datas_num] = rtt_get_counter() - counter;

					fcf_mhr[datas_num * 2] = mhr[0];
					fcf_mhr[datas_num * 2 + 1] = mhr[1];


					datas_num++;
				}
				else
				{
					DEBUG2("awouèèèèèèèèèèèèè\n");
				}
				// each_round_first = true;
				// DEBUG2("LA TAILLE DE LA LISTE DES NOEUD EST %d DANS LE SECTEUR %d\n",collect_list_size[((collect_sector - 1) % GNRC_WAYE_SECTORS)],(collect_sector - 1) % GNRC_WAYE_SECTORS);
				// DEBUG2("THE NUMBER OF SLOT IS : %d\n",slot);
				// gnrc_waye_send_bitmap_ack(netif, &data_ack, gack_bitmap, gack_bitmap_size);
				/*
									if (slot >= 0) {
										set_bit(gack_bitmap, slot);
									}
									else {
										DEBUG2("[WAYE]: Data reply wrong slot !\n");
									}
				*/
			}
			else
			{
				// uint32_t counter = rtt_get_counter();
				gnrc_pktsnip_t *pkt = netif->ops->recv(netif);
				DEBUG("[WAYE] NETDEV_EVENT_RX_COMPLETE\n");

				if (pkt)
				{
					if (nb_discovery_phase)
					{
						if (waiting_hellos)
						{
							uint8_t *ptr = pkt->data;

							if (*ptr == GNRC_WAYE_FRAMETYPE_HELLO)
							{
								if (hellos_num < GNRC_WAYE_HELLO_QUEUE_SIZE)
								{
									hellos[hellos_num].pkt = pkt;
									hellos[hellos_num].rx_start_counter = last_rx_start_counter;
									hellos[hellos_num].rx_end_counter = counter;
									hellos_num++;

									uint8_t addresses = sizeof(gnrc_waye_frame_hello_t);
									for( ; addresses < pkt->size ; addresses = addresses + 2 )
									{
										
									}
								}
								else
								{
									gnrc_pktbuf_release(pkt);
									DEBUG("WARNING [WAYE]: HELLO queue full !\n");
								}
							}
							else
							{
								/* Drop non-hello packet */
								// non_hellos[non_hellos_num].pkt = pkt;
								non_hellos[non_hellos_num].rx_start_counter = last_rx_start_counter;
								non_hellos[non_hellos_num].rx_end_counter = counter;
								non_hellos_num++;
								gnrc_pktbuf_release(pkt);
								// DEBUG2("rx start counter : %lu rx end counter %lu\n",TICKS_TO_US(non_hellos[non_hellos_num].rx_start_counter),TICKS_TO_US(non_hellos[non_hellos_num].rx_end_counter));
								DEBUG2("[WAYE]: Non-hello packet dropped\n");
							}
						}
						else
						{
							/* Drop packet if received too late */
							gnrc_pktbuf_release(pkt);
							DEBUG2("[WAYE]: Dropped packet received after end of round\n");
						}
					}
					else
					{
						gnrc_pktbuf_release(pkt);
						DEBUG("[WAYE]: Dropped packet\n");
					}
				}

				gnrc_netif_set_rx_started(netif, false);

				if (nb_discovery_phase && waiting_hellos)
				{
					at86rf2xx_wait_medium_busy((at86rf2xx_t *)dev);
				}
			}
			break;

		case NETDEV_EVENT_TX_STARTED:

			DEBUG("[WAYE] NETDEV_EVENT_TX_STARTED\n");
			gnrc_netif_set_tx_feedback(netif, TX_FEEDBACK_UNDEF);
			gnrc_netif_set_rx_started(netif, false);
			break;

		case NETDEV_EVENT_TX_COMPLETE:
			DEBUG("[WAYE] NETDEV_EVENT_TX_COMPLETE\n");
			if (!is_sent_ack)
			{
				if (nb_discovery_phase)
				{

					waye_tx_end_counter = rtt_get_counter() + RTT_US_TO_TICKS(GNRC_WAYE_ND_SLOT_OFFSET_US);
					waye_tx_res = event;
					uint32_t alarm = waye_tx_end_counter + RTT_US_TO_TICKS(GNRC_WAYE_ND_SLOT_SIZE_US * waye_frame.nb_slot);
					rtt_set_alarm(alarm, waye_round_end_cb, NULL);
				}
				else if (collection_phase)
				{
					if (sent_emergency)
					{
						poll_tx_end_counter = rtt_get_counter() + RTT_US_TO_TICKS(GNRC_WAYE_COLLECT_SLOT_OFFSET_US);
						uint32_t alarm = poll_tx_end_counter + RTT_US_TO_TICKS(GNRC_WAYE_COLLECT_SLOT_SIZE_US);
						rtt_set_alarm(alarm, waye_collect_end_cb, NULL);
					}
					else
					{
						poll_tx_end_counter = rtt_get_counter() + RTT_US_TO_TICKS(GNRC_WAYE_COLLECT_SLOT_OFFSET_US);
						uint32_t alarm = poll_tx_end_counter + RTT_US_TO_TICKS(GNRC_WAYE_COLLECT_SLOT_SIZE_US * (collect_list_size[collect_sector] + 1));
						rtt_set_alarm(alarm, waye_collect_end_cb, NULL);
						//collect_sector = (collect_sector + 1) % GNRC_WAYE_SECTORS;
					}
				}
				gnrc_netif_set_tx_feedback(netif, TX_FEEDBACK_SUCCESS);
				gnrc_netif_set_rx_started(netif, false);
				if (nb_discovery_phase)
				{
					waiting_hellos = true;
					waye_cca_loop = true;
					waye_ed_loop = true;
					memset(rssi, RSSI_BASE_VAL, sizeof(rssi));
					at86rf2xx_wait_medium_busy((at86rf2xx_t *)dev);
				}
			}
			break;

		case NETDEV_EVENT_TX_NOACK:
			DEBUG("[WAYE] NETDEV_EVENT_TX_NOACK\n");
			gnrc_netif_set_tx_feedback(netif, TX_FEEDBACK_NOACK);
			gnrc_netif_set_rx_started(netif, false);
			break;

		case NETDEV_EVENT_TX_MEDIUM_BUSY:
			DEBUG("[WAYE] NETDEV_EVENT_TX_MEDIUM_BUSY\n");
			if (nb_discovery_phase)
			{
				waye_tx_res = event;
			}
			gnrc_netif_set_tx_feedback(netif, TX_FEEDBACK_BUSY);
			gnrc_netif_set_rx_started(netif, false);
			break;

		default:
			DEBUG("WARNING [WAYE]: Unhandled event %u.\n", event);
		}
	}
}

static void waye_neighbor_discovery(gnrc_netif_t *netif)
{
	nb_discovery_phase = true;
	waiting_hellos = false;
	last_rx_start_counter = 0;
	rx_start_num = 0;
	hellos_num = 0;
	non_hellos_num = 0;
	medium_busy_num = 0;

	waye_frame.discovery_id++;
	waye_frame.sector = 0;
	waye_frame.round = 0;
	waye_frame.nb_slot = GNRC_WAYE_INITIAL_SLOTS;
	waye_frame.p = 1.0;
	waye_frame.slot_size_ms = GNRC_WAYE_ND_SLOT_SIZE_MS;

	/* Start with sector 0 */
	sba_sp6t_set_beam(&sba, 0);
	gnrc_waye_send_waye(netif, (gnrc_waye_frame_waye_t *)&waye_frame, ack_addr_list[waye_frame.sector], ack_addr_list_size[waye_frame.sector]);
}

static void _waye_msg_handler(gnrc_netif_t *netif, msg_t *msg)
{
	switch (msg->type)
	{
	case GNRC_WAYE_EVENT_ROUND_END:
		waye_round_end_handler(netif);
		break;

	case GNRC_WAYE_EVENT_COLLECT_END:
		waye_collect_end_handler(netif);
		break;

	case GNRC_WAYE_EVENT_INIT_END:
		waye_neighbor_discovery(netif);
		break;

	default:
		DEBUG("[WAYE]: unknown message type 0x%04x"
			  "(no message handler defined)\n",
			  msg->type);
	}
}

static void waye_init_cb(void *arg)
{
	msg_t msg;

	msg.content.ptr = arg;
	msg.type = GNRC_WAYE_EVENT_INIT_END;
	msg_send(&msg, waye_pid);

	if (sched_context_switch_request)
	{
		thread_yield();
	}
}

static int _waye_init(gnrc_netif_t *netif)
{
	netdev_t *dev;
	int res;
	dev = netif->dev;
	dev->event_callback = _waye_event_cb;
	dev->context = netif;

	/* Store pid globally, so that IRQ can use it to send msg */
	waye_pid = netif->pid;

	if (dev->driver->init(dev) < 0)
	{
		DEBUG("[WAYE]PROBLEM DRIVER INIT\n");
	}

	/* Initialize RTT. */
	rtt_init();
	/* Initialize the Switched-Beam Antenna */

	sba_sp6t_params_t sba_params;
	sba_params.control1 = GPIO_PIN(PB, 23);
	sba_params.control2 = GPIO_PIN(PB, 2);
	sba_params.control3 = GPIO_PIN(PB, 22);
	res = sba_sp6t_init(&sba, &sba_params);

	if (res == 0)
	{
		DEBUG("[WAYE] Switched beam antenna initialized successfully\n");
	}
	else
	{
		DEBUG("WARNING: [WAYE] Switched beam antenna initialization failed\n");
	}

	/* Enable RX_START and TX_END interrupts */
	netopt_enable_t enable = NETOPT_ENABLE;
	dev->driver->set(dev, NETOPT_RX_START_IRQ, &enable, sizeof(enable));
	dev->driver->set(dev, NETOPT_TX_END_IRQ, &enable, sizeof(enable));
	dev->driver->set(dev, NETOPT_PROMISCUOUSMODE, &enable, sizeof(enable));
	dev->driver->set(dev, NETOPT_AUTOACK, &enable, sizeof(enable));

	/* Disable CSMA/CA */
	enable = NETOPT_DISABLE;
	dev->driver->set(dev, NETOPT_CSMA, &enable, sizeof(enable));

	netdev_ieee802154_t *device_state = container_of(dev, netdev_ieee802154_t, netdev);
	at86rf2xx_t *at86rf2xx_dev = container_of(device_state, at86rf2xx_t, netdev);

	uint8_t tmp = at86rf2xx_reg_read(at86rf2xx_dev, AT86RF2XX_REG__IRQ_MASK);
	tmp |= AT86RF2XX_IRQ_STATUS_MASK__AMI;
	at86rf2xx_reg_write(at86rf2xx_dev, AT86RF2XX_REG__IRQ_MASK, tmp);

#if defined(MODULE_AT86RF233) || defined(MODULE_AT86RF231)
	/* Seed RNG with random from AT86RF2XX */
	uint32_t seed;
	at86rf2xx_get_random(at86rf2xx_dev, (uint8_t *)&seed, sizeof(seed));
	random_init(seed);
#endif

	/* Initialize IEEE 802.15.4 sequence number with random value */
	random_bytes(&device_state->seq, sizeof(device_state->seq));

	nb_discovery_phase = false;

	/* Initialize the WAYE discovery ID with random value */
	random_bytes((uint8_t *)&waye_frame.discovery_id, sizeof(waye_frame.discovery_id));

	/* Let time for the RTT to stabilize */
	DEBUG2("[WAYE] irq mask = %u\n", at86rf2xx_reg_read(at86rf2xx_dev, AT86RF2XX_REG__IRQ_MASK));

	tmp = at86rf2xx_reg_read(at86rf2xx_dev, AT86RF2XX_REG__CSMA_SEED_1);
	tmp |= AT86RF2XX_CSMA_SEED_1__AACK_FVN_MODE;
	at86rf2xx_reg_write(at86rf2xx_dev, AT86RF2XX_REG__CSMA_SEED_1, tmp);
	DEBUG2("CSMA_SEED_1 REG : 0x%x\n", at86rf2xx_reg_read(at86rf2xx_dev, AT86RF2XX_REG__CSMA_SEED_1));

	be_uint16_t full_sink_addr;
	at86rf2xx_get_addr_short(at86rf2xx_dev, &full_sink_addr);
	sink_addr[0] = full_sink_addr.u8[0];
	sink_addr[1] = full_sink_addr.u8[1];
	for(unsigned i = 0 ; i < GNRC_WAYE_SECTORS ; i++){
		ack_addr_list_size[i]  = 0;
	}

	rtt_set_alarm(RTT_SEC_TO_TICKS(2), waye_init_cb, NULL);

	return 0;
}

static void waye_round_begin(gnrc_netif_t *netif, uint8_t sector, uint8_t nb_slot, float p)
{
	rx_start_num = 0;
	hellos_num = 0;
	non_hellos_num = 0;
	medium_busy_num = 0;

	if (sector == waye_frame.sector)
	{
		waye_frame.round++;
	}
	else
	{
		waye_frame.round = 0;
		sba_sp6t_set_beam(&sba, sector);
	}

	waye_frame.sector = sector;
	waye_frame.nb_slot = nb_slot;
	waye_frame.p = p;
	waye_frame.slot_size_ms = GNRC_WAYE_ND_SLOT_SIZE_MS;

	gnrc_waye_send_waye(netif, (gnrc_waye_frame_waye_t *)&waye_frame, ack_addr_list[waye_frame.sector], ack_addr_list_size[waye_frame.sector]);
}

static inline int waye_hello_slot(uint32_t waye_tx_end_ticks, uint32_t hello_rx_end_ticks)
{
	uint32_t us = TICKS_TO_US(hello_rx_end_ticks - waye_tx_end_ticks) - GNRC_WAYE_BUSY_RX_END_US;
	us /= GNRC_WAYE_ND_SLOT_SIZE_US;
	return us < waye_frame.nb_slot ? (int)us : -1;
}

static inline int waye_rx_start_slot(uint32_t waye_tx_end_ticks, uint32_t rx_start_ticks)
{
	uint32_t us = TICKS_TO_US(rx_start_ticks - waye_tx_end_ticks) - GNRC_WAYE_BUSY_RX_START_US;
	us /= GNRC_WAYE_ND_SLOT_SIZE_US;
	return us < waye_frame.nb_slot ? (int)us : -1;
}

static inline int waye_coll_slot(uint32_t waye_tx_end_ticks, uint32_t medium_busy_ticks)
{
	uint32_t us = TICKS_TO_US(medium_busy_ticks - waye_tx_end_ticks);
	us /= GNRC_WAYE_ND_SLOT_SIZE_US;
	return us < waye_frame.nb_slot ? (int)us : -1;
}

static inline int data_reply_slot(uint32_t poll_tx_end_ticks, uint32_t rx_start_ticks)
{
	uint32_t us = TICKS_TO_US(rx_start_ticks - poll_tx_end_ticks); //- GNRC_WAYE_BUSY_RX_START_US
	us /= GNRC_WAYE_COLLECT_SLOT_SIZE_US;
	return us;
}

static void waye_round_end_cb(void *arg)
{
	waiting_hellos = false;
	waye_continue_rssi_loop = false;

	waye_cca_loop = false;

	msg_t msg;

	msg.content.ptr = arg;
	msg.type = GNRC_WAYE_EVENT_ROUND_END;
	msg_send(&msg, waye_pid);

	if (sched_context_switch_request)
	{
		thread_yield();
	}
}

static void waye_round_end_handler(void *arg)
{
	gnrc_netif_t *netif = (gnrc_netif_t *)arg;

	// at86rf2xx_t *dev = (at86rf2xx_t *)arg;
	// at86rf2xx_set_state(dev, AT86RF2XX_STATE_TRX_OFF);
	// DEBUG2("nombre de rssi = %lu, nombre rssi dif BAS_VAL = %lu, temps du round = %lu\n",nbrssi,nbrssidif,TICKS_TO_US(rtt_get_counter() - waye_tx_end_counter));

	gnrc_waye_slot_state_t slot_state[waye_frame.nb_slot];
	for (unsigned i = 0; i < waye_frame.nb_slot; i++)
	{
		slot_state[i] = 0;
	}

	// rssi_fill_time[fill_counter] =  TICKS_TO_US(waye_tx_end_counter - RTT_US_TO_TICKS(GNRC_WAYE_ND_SLOT_OFFSET_US));
	fill_counter++;
	// rssi_fill_time[fill_counter] = rssi_time[nbrssidif-1];
	fill_counter++;

	nbed = 0;
	nround++;

	DEBUG2("\n[WAYE] Results :\nReceived hellos :\n");
	if (wcrl_in_waye > 1)
	{
		DEBUG2("WCRL CAME FROM WAYE \n");
	}
	for (unsigned i = 0; i < hellos_num; i++)
	{
		int slot = waye_rx_start_slot(waye_tx_end_counter, hellos[i].rx_start_counter);

		DEBUG2("%luµs - %luµs   (slot %d)\n", TICKS_TO_US(hellos[i].rx_start_counter - waye_tx_end_counter), TICKS_TO_US(hellos[i].rx_end_counter - waye_tx_end_counter), slot);
		fflush(stdout);
		// od_hex_dump(hellos[i].pkt->data, hellos[i].pkt->size, 0);

		uint8_t *ptr = hellos[i].pkt->data;

		/* Using pkt size contradict the use of collaboration */
		if (*ptr == GNRC_WAYE_FRAMETYPE_HELLO && slot >= 0)
		{
			gnrc_waye_frame_hello_t *hello = (gnrc_waye_frame_hello_t *)(ptr + 1);
			DEBUG2("SIZE OF THE PACKET IS : %d\n", hellos[i].pkt->size);
			DEBUG2("le discovery ID est : %d the expected discovery ID is : %d\n", hello->discovery_id, waye_frame.discovery_id);
			DEBUG2("le secteur est : %d the expected sector is : %d\n", hello->sector, waye_frame.sector);
			DEBUG2("le round est : %d the expected round is : %d\n", hello->round, waye_frame.round);

			/* If HELLO corresponds to the current round */
			if (hello->discovery_id == waye_frame.discovery_id && hello->sector == waye_frame.sector && hello->round == waye_frame.round)
			{
				DEBUG2("Hello : discovery_id = %u  sector = %u  round = %u  slot = %u  RSSI = %d  LQI = %u  addr = 0x%04x", hello->discovery_id, hello->sector, hello->round, hello->selected_slot, hello->rssi_waye, hello->lqi_waye, hello->addr.u16);

				if (slot != hello->selected_slot)
				{
					DEBUG2(" =/=\n");
				}
				else
				{
					DEBUG2("\n");
				}

				if (hellos[i].pkt->next)
				{
					gnrc_netif_hdr_t *hdr = hellos[i].pkt->next->data;

					DEBUG2("RSSI = %d  LQI = %u\n", hdr->rssi, hdr->lqi);

					waye_conn_table_add(hello->addr, hello->sector, hello->rssi_waye, hdr->rssi, hello->lqi_waye, hdr->lqi, hello->node_type);

					if (ack_addr_list_size[waye_frame.sector] < GNRC_WAYE_ACK_ADDR_LIST_SIZE)
					{
						ack_addr_list[waye_frame.sector][ack_addr_list_size[waye_frame.sector]] = hello->addr;
						ack_addr_list_size[waye_frame.sector]++;
					}
					else
					{
						DEBUG("WARNING [WAYE]: HELLO ACK list full !\n");
					}

					if (slot_state[slot] == SLOT_EMPTY)
					{
						slot_state[slot] = SLOT_SUCCESS;
					}
					else
					{
						DEBUG2("COLLISION !\n");
						slot_state[slot] = SLOT_COLLISION;
					}
				}
			}
		}
	}
	// DEBUG("Number of non-hellos messages in this round is %d\n",non_hellos_num);
	for (unsigned i = 0; i < non_hellos_num; i++)
	{
		//int slot2 = waye_rx_start_slot(waye_tx_end_counter, non_hellos[i].rx_start_counter);
		DEBUG2("non-hellos rx start_counter started at : %lu\n", TICKS_TO_US(non_hellos[i].rx_start_counter));
		DEBUG2("tx ended at :%lu\n", TICKS_TO_US(waye_tx_end_counter));
		DEBUG2("non-hellos rx end_counter ended at : %lu\n", TICKS_TO_US(non_hellos[i].rx_end_counter));

	}

	//	DEBUG2("RX_START timestamps :\n");
	for (unsigned i = 0; i < rx_start_num; i++)
	{
		int slot = waye_rx_start_slot(waye_tx_end_counter, rx_start_counters[i]);

		if (slot >= 0 && slot_state[slot] == SLOT_EMPTY)
		{
			slot_state[slot] = SLOT_COLLISION;
		}
		// DEBUG2("%luµs - %luµs  \n", TICKS_TO_US(hellos[i].rx_start_counter - waye_tx_end_counter), TICKS_TO_US(hellos[i].rx_end_counter - waye_tx_end_counter));
		// fflush(stdout);
		// DEBUG2("rx started at : %lu µs \t", TICKS_TO_US(rx_start_counters[i]));
		// DEBUG2("tx ended at :%lu",TICKS_TO_US(waye_tx_end_counter));
		// DEBUG2("number of reception in this round is %d size table : %lu \n", rx_start_num,nbrssidif);//TICKS_TO_US(rx_start_counters[i] - waye_tx_end_counter)
	}

	DEBUG2("Medium busy timestamps :\n");
	for (unsigned i = 0; i < medium_busy_num; i++)
	{
		int slot = waye_coll_slot(waye_tx_end_counter, medium_busy_counters[i]);

		if (slot >= 0 && slot_state[slot] == SLOT_EMPTY)
		{
			slot_state[slot] = SLOT_COLLISION;
		}

		DEBUG2("%luµs   (slot %d)\n", TICKS_TO_US(medium_busy_counters[i] - waye_tx_end_counter), slot);
	}

	/* Release pkt buffers */
	for (unsigned i = 0; i < hellos_num; i++)
	{
		gnrc_pktbuf_release(hellos[i].pkt);
	}
	for (unsigned i = 0; i < non_hellos_num; i++)
	{
		gnrc_pktbuf_release(non_hellos[i].pkt);
	}

	printf("RX_START_TIME : %lu µs (last rx_start = %lu, tx_end_count = %lu  \n", TICKS_TO_US(last_rx_start_counter - waye_tx_end_counter),
		   TICKS_TO_US(last_rx_start_counter),
		   TICKS_TO_US(waye_tx_end_counter));

	hellos_num = 0;
	non_hellos_num = 0;
	rx_start_num = 0;
	medium_busy_num = 0;
	last_rx_start_counter = 0;
	wcrl_in_waye = 0;

	unsigned nb_slot_empty = 0;
	unsigned nb_slot_success = 0;
	unsigned nb_slot_collision = 0;

	for (unsigned i = 0; i < waye_frame.nb_slot; i++)
	{
		switch (slot_state[i])
		{
		case SLOT_EMPTY:
			nb_slot_empty++;
			break;

		case SLOT_SUCCESS:
			nb_slot_success++;
			break;

		default:
			nb_slot_collision++;
		}
	}
	uint8_t nb_rssi_per_round = 200;
	/*
	if (nbrssidif + nb_rssi_per_round >= GNRC_WAYE_RSSI_TABLES_SIZE)
	{
		for (unsigned i = 0; i < nbrssidif; i++)
		{
			DEBUG2("%d ", rssi_ed[i]);
		}
		DEBUG2("\n");
		for (unsigned i = 0; i < nbrssidif; i++)
		{
			DEBUG2("%lu ", rssi_time[i]);
		}
		DEBUG2("\n");
		fflush(stdout);
		nbrssidif = 0;
	}
	*/
	// DEBUG2("S0 = %u   S1 = %u   S2* = %u\n", nb_slot_empty, nb_slot_success, nb_slot_collision);
	int estimation = waye_estimate_remaining(nb_slot_empty, nb_slot_success, nb_slot_collision);
	// DEBUG2("Estimation restants : %d\n", estimation);
	/* If all slots are empty or estimation = 0 */
	if (nb_slot_empty == waye_frame.nb_slot || estimation <= 0)
	{
		/* If this round had 1 slot and it was empty */
		if (waye_frame.nb_slot == 1 && nb_slot_empty == 1U)
		{
			/* If sector remaining */
			if (waye_frame.sector + 1U < GNRC_WAYE_SECTORS)
			{
				/* Begin round on next sector */
				/* But first empty the address list so that nodes are discovered in each sector */
/*
				for (unsigned int i = 0; i < ack_addr_list_size; i++)
				{
					ack_addr_list[i].u16 = 0;
				}
*/
				waye_round_begin(netif, waye_frame.sector + 1U, GNRC_WAYE_INITIAL_SLOTS, 1.0);
			}
			else
			{
				uint32_t discovery_phase_end = TICKS_TO_US(rtt_get_counter());
				/* End neighbor discovery */
				ztimer_sleep(ZTIMER_USEC,1000);
				nb_discovery_phase = false;
				/*
				for (unsigned i = 0; i < nbrssidif; i++)
				{
					DEBUG2("%d ", rssi_ed[i]);
				}
				DEBUG2("\n");
				for (unsigned i = 0; i < nbrssidif; i++)
				{
					DEBUG2("%lu ", rssi_time[i]);
				}
				DEBUG2("\n");
				*/
				nbrssidif = 0;
				if (first_discovery)
				{
					first_discovery = false;
					ztimer_sleep(ZTIMER_SEC,1);
					waye_neighbor_discovery(netif);
				}
				else
				{

					waye_conn_table_print_sectors();
					waye_conn_table_print_nodes();
					DEBUG2("Nombre de round = %u, nombre de rssi = %lu, nombre rssi dif BAS_VAL = %lu, temps de la phase de découverte = %lu\n", nround, nbrssi, nbrssidif, discovery_phase_end);
					// for(unsigned i = 0 ; i < fill_counter ; i++){
					//	DEBUG2("%lu ",rssi_fill_time[i]);
					// }
					DEBUG2("\n");

					if (waye_conn_table_nodes_available())
					{
						waye_conn_table_update_best_sector(SECTOR_POLICY_MEAN);
						waye_collect_init();

						waye_collect(netif);
					}
				}
			}
		}
		else
		{
			/* Do one last round with 1 slot for safety */
			waye_round_begin(netif, waye_frame.sector, 1, 1.0);
		}
	}
	else
	{
		/* Do next round with nb_slot = estimation with 255 slot limit */
		waye_round_begin(netif, waye_frame.sector, estimation <= 255 ? estimation : 255, 1.0);
	}
}

static void at86rf2xx_wait_medium_busy(at86rf2xx_t *dev)
{
	uint32_t time_rssi = 0;

	/* Initialize table to RSSI_BASE_VAL */

	DEBUG("[WAYE] Starting RSSI loop\n");
	do
	{
		rssi[pos] = at86rf2xx_get_rssi(dev);
		time_rssi = rtt_get_counter();
		//rssi_time[nbrssidif] = TICKS_TO_US(time_rssi);
		//rssi_ed[nbrssidif] = rssi[pos];
		nbrssidif++;
		nbrssi++;
		rssi_avg = 0;

		for (unsigned i = 0; i < GNRC_WAYE_RSSI_NUMBER; i++)
		{
			rssi_avg += rssi[i];
		}
		rssi_avg /= (int)GNRC_WAYE_RSSI_NUMBER;

		if (rssi_avg > GNRC_WAYE_RSSI_THRESHOLD)
		{
			if (medium_busy_num < GNRC_WAYE_BUSY_QUEUE_SIZE)
			{
				medium_busy_counters[medium_busy_num] = time_rssi;

				medium_busy_num++;
			}
			else
			{
				DEBUG("WARNING [WAYE]: Medium busy queue full !\n");
			}
		}

		pos = (pos + 1) % GNRC_WAYE_RSSI_NUMBER;
	} while (waye_continue_rssi_loop);
	if (waiting_hellos)
	{
		wcrl_in_waye++;
	}
	DEBUG("[WAYE] Stopping RSSI loop\n");
}

static inline int waye_estimate_remaining(unsigned nb_slot_empty, unsigned nb_slot_success, unsigned nb_slot_collision)
{
	double estimation;
	unsigned nb_slot = nb_slot_empty + nb_slot_success + nb_slot_collision;

	estimation = (1.5 - nb_slot_empty / (double)nb_slot) * 2.0 * nb_slot_collision;

	return round(estimation);
}

static void waye_collect_init(void)
{
	gnrc_waye_conn_table_entry_t *ptr;
	waye_global_collection_phase = true;
	
	collect_sector = 0;

	for (unsigned i = 0; i < GNRC_WAYE_SECTORS; i++)
	{
		collect_list_size[i] = 0;
	}

	uint8_t sector = 1;
	nb_rx_start = 0;
	for(unsigned int incr = 0 ; incr < GNRC_WAYE_SECTORS ; incr++){
		standard_queue[incr] = NULL;
		standard_queue_size[incr] = 0;
	}

	uint8_t nodes = waye_conn_table_nodes_available();
	uint8_t max_node = floor((double) nodes / GNRC_WAYE_SECTORS) + 1;
	uint8_t tableau[12] = {0,0,0,1,1,1,2,2,4,4,5,5};
	uint8_t index = 0;
	for (ptr = waye_conn_table_get_head(); ptr != NULL; ptr = ptr->next)
	{
		/* Changing the sector */
		bool placed = false;
		ptr->best_sector = sector % GNRC_WAYE_SECTORS;
		sector++;
		

		
		ptr->best_sector = tableau[index];
		index++;
		DEBUG2(" Best Sector for node %02x : %u\n",ptr->addr.u16,ptr->best_sector);
		if(ptr->node_type != 1){
			actuator_list[ptr->best_sector][actuator_list_size[ptr->best_sector]] = ptr->addr;
			actuator_list_size[ptr->best_sector]++;
		}
		if(ptr->node_type != 0){
			collect_list[ptr->best_sector][collect_list_size[ptr->best_sector]] = ptr->addr;
			collect_list_size[ptr->best_sector]++;
		}

		/**
		 * Si vous souhaitez que les noeuds soient placés aléatoirement dans les secteurs,
		 * décommentez la boucle while suivante.
		*/

		/*
		while(placed == false){
			uint32_t chosed_sector = random_uint32_range(0,GNRC_WAYE_SECTORS);
			if(actuator_list_size[chosed_sector] + collect_list_size[chosed_sector] < max_node){
				ptr->best_sector = chosed_sector;
				placed = true;
			}
		}
		*/
	}

	data_poll.poll_id = 255;
	data_poll.slot_size_ms = GNRC_WAYE_COLLECT_SLOT_SIZE_MS;
	data_mixed_poll.slot_size_ms = data_poll.slot_size_ms;


	thread_create(queue_generator_thread_stack, sizeof(queue_generator_thread_stack), THREAD_PRIORITY_MAIN + 2, THREAD_CREATE_STACKTEST, _thread_add_element_to_queue, NULL, "queue-generation-thread");
}

static void waye_collect(gnrc_netif_t *netif)
{
	temps_debut_collecte = rtt_get_counter();
	data_poll.poll_id++;
	collection_phase = true;

	if (emergency_queue_size > 0)
	{
		//DEBUG2("EMERGENCY \n");
		sba_sp6t_set_beam(&sba, emergency_queue->node_sector);
		
		sent_emergency = true;
		is_sent_ack = false;

		if(emergency_queue->frame_type == GNRC_WAYE_FRAMETYPE_ACTUATOR_ORDER) {
			//DEBUG2("Order for %4x\n",emergency_queue->node_addr.u16);
			gnrc_waye_send_actuator_order(netif, &emergency_queue->actuator_order, 1, sink_addr);
		}
		if(emergency_queue->frame_type == GNRC_WAYE_FRAMETYPE_DATA_POLL) {
			DEBUG2("Poll\n");
			gnrc_waye_send_data_poll(netif, (gnrc_waye_frame_data_poll_t *)&data_poll, &emergency_queue->node_addr, 1, sink_addr);
		}
	}
	else
	{
		collect_sector = (collect_sector + 1) % GNRC_WAYE_SECTORS;
		for (; collect_list_size[collect_sector] == 0 && standard_queue_size[collect_sector] == 0 ; collect_sector = (collect_sector + 1) % GNRC_WAYE_SECTORS)
		{
		}
		sba_sp6t_set_beam(&sba, collect_sector);
		//DEBUG2("[COLLECT] Sector : %u\n",collect_sector);
		if(standard_queue_size[collect_sector] != 0){
			//DEBUG2("SENDING TO NODE standard %02x\n", standard_queue[collect_sector]->actuator_order.addr.u16);

			gnrc_waye_frame_actuator_order_t sending_order[standard_queue_size[collect_sector]];
			gnrc_waye_waiting_node_t * transmission = standard_queue[collect_sector];
			for(unsigned int f = 0 ; f < standard_queue_size[collect_sector] ; f++){
				sending_order[f] = transmission->actuator_order;
				transmission = transmission->next;
			}
			if(collect_list_size[collect_sector] != 0){

				data_mixed_poll.poll_id = data_poll.poll_id;
				data_mixed_poll.list_poll_size = collect_list_size[collect_sector];

				sent_standard = true;
				is_sent_ack = false;
				gnrc_waye_send_mixed_poll(netif, (gnrc_waye_frame_data_mixed_poll_t *)&data_mixed_poll, collect_list[collect_sector], sending_order, standard_queue_size[collect_sector],sink_addr);
			}
			else {
				sent_standard = true;
				is_sent_ack = false;

				gnrc_waye_send_actuator_order(netif, &standard_queue[collect_sector]->actuator_order, standard_queue_size[collect_sector], sink_addr);
			}
			//DEBUG2("SENDING ON SECTOR standard %d\n", collect_sector);
			//DEBUG2("SENDING TO NODE standard %02x, %02x, %02x %u\n", sending_order[0].addr.u16, standard_queue[collect_sector]->actuator_order.addr.u16,trans->node_addr.u16,standard_queue_size[collect_sector]);
		}
		if((standard_queue_size[collect_sector] == 0) || (standard_queue[collect_sector]->node_sector != collect_sector)){

			gack_bitmap_size = ((collect_list_size[collect_sector] - 1) / 8) + 1;
			memset(gack_bitmap, 0, gack_bitmap_size);

			//DEBUG2("SENDING ON SECTOR default %d\n", collect_sector);

			is_sent_ack = false;
			gnrc_waye_send_data_poll(netif, (gnrc_waye_frame_data_poll_t *)&data_poll, collect_list[collect_sector], collect_list_size[collect_sector], sink_addr);
		}
	}
}

static void waye_collect_end_cb(void *arg)
{
	msg_t msg;

	msg.content.ptr = arg;
	msg.type = GNRC_WAYE_EVENT_COLLECT_END;
	msg_send(&msg, waye_pid);

	if (sched_context_switch_request)
	{
		thread_yield();
	}
}

static void waye_collect_end_handler(void *arg)
{
	gnrc_netif_t *netif = (gnrc_netif_t *)arg;

	// gnrc_waye_frame_data_ack_t data_ack;
	// data_ack.poll_id = data_poll.poll_id;

	temps_fin_collecte = rtt_get_counter();
	if(collect_sector == 2){
		DEBUG2("\nEnd collect HANDLER\n");
	}
	
	nbcollect++;
	
	for (int i = 0; i < datas_num; i++)
	{
		//DEBUG2("\n\ndata queue len : %u || numero de sequence : %u || temps collecte : %lu || crcs : %d\n", datas[i].pkt.data_queue_len, datas[i].pkt.seq_num, TICKS_TO_US(temps_collecte[i]), crcs[i]);
		// for(int j = 0 ; j < datas[i].pkt.data_queue_len ; j++){
		//	DEBUG2("%u à %u ][ ", datas[i].pkt.data_queue[j].data, datas[i].pkt.data_queue[j].timestamp);
		// }
		//DEBUG2("NUMERO SEQUENCE : %u\n", numero_de_sequence[i]);
		//DEBUG2("FCF_MHR : 0x%x%x\n", fcf_mhr[i * 2], fcf_mhr[i * 2 + 1]);
	}

	// DEBUG2("Temps round collecte : %lu, FCF_MHR : 0x%x%x\n",TICKS_TO_US(temps_fin_collecte-temps_debut_collecte),fcf_mhr[0],fcf_mhr[1]);
	//DEBUG2("\nTemps rx_start -> tx_end : %lu, NB RX START : %u\n", TICKS_TO_US(poll_tx_end_counter - last_rx_start_counter), nb_rx_start);

	memset(original_frame,0,72);

	for (int i = 0; i < 24; i++)
	{
		memset(matrix[i], 0, 24);
	}

	datas_num = 0;
	if((emergency_queue_size >= 1) && (sent_emergency == true)) {
		//DEBUG2("reducing emergency queue, standard sec1 %02x, standard sec2 %02x\n",standard_queue[1]->actuator_order.addr.u16,standard_queue[2]->actuator_order.addr.u16);
		gnrc_waye_waiting_node_t * free_memory_pointer = emergency_queue;
		emergency_queue = emergency_queue->next;
		free(free_memory_pointer);
		emergency_queue_size--;
	}

	if((standard_queue_size[collect_sector] >= 1) && (sent_standard == true)) {
		gnrc_waye_waiting_node_t * free_memory_pointer = standard_queue[collect_sector];
		standard_queue[collect_sector] = NULL;
		//DEBUG2("reducing standard queue free ? %02x, order %x\n",free_memory_pointer->actuator_order.addr.u16, free_memory_pointer->actuator_order.order);
		free(free_memory_pointer);
		standard_queue_size[collect_sector] = 0;
	}

	if(nb_event % 50 == 0){
		unsigned nodes = waye_conn_table_nodes_available();
		uint32_t random_node = random_uint32_range(0, nodes);
		gnrc_waye_conn_table_entry_t *ptr = waye_conn_table_get_head();

		for(unsigned int i = 0 ; i < random_node ; i++) {
			ptr = ptr->next;
		}

		int8_t result = add_queue_element(ptr->addr, &emergency_queue, emergency_queue_size);
		if(result < 0){
			DEBUG2("PANIC PANIC PANIC\n \n \n");
		}
		emergency_queue_size = result;
		DEBUG2("Order %u => CP Emergency %02x\n", order_id - 1, ptr->addr.u16);
		DEBUG2("nbevent : %u\n",nb_event);
		//DEBUG2("CP Emergency %02x, standard sec1 %02x, standard sec2 %02x\n",ptr->addr,standard_queue[1]->actuator_order.addr.u16,standard_queue[2]->actuator_order.addr.u16);
		emergency_queue->actuator_order.order ^= 0x10;
		nb_event++;
	}
	/*
	else {
		unsigned nodes = waye_conn_table_nodes_available();
		uint32_t random_node = random_uint32_range(0, nodes);
		gnrc_waye_conn_table_entry_t *ptr = waye_conn_table_get_head();

		for(uint32_t i = 0 ; i < random_node ; i++) {
			ptr = ptr->next;
		}

		int8_t res = add_queue_element(ptr->addr, &standard_queue[ptr->best_sector], standard_queue_size[ptr->best_sector]);
		if(res < 0){
			DEBUG2("PANIC PANIC PANIC\n \n \n");
		}
		standard_queue_size[ptr->best_sector] = res;
		DEBUG2("CP Fil action fil eau %02x \n",ptr->addr.u16);
	}
	nb_event = 1001;
	*/
	nb_event++;
	if (nb_event <= 1000)
	{
		//ztimer_sleep(ZTIMER_MSEC,100);
		//DEBUG2("\n[WAYE] ROUND COLLECT !\n");
		
		sent_emergency = false;
		sent_standard = false;
		waye_collect(netif);
	}
	else if (nb_event > 1000) { DEBUG2("\n\n[WAYE] FIN\n"); }
}


static inline void set_bit(uint8_t bitmap[], unsigned bit)
{
	bitmap[bit / 8] |= 1 << (bit % 8);
}

static inline void clear_bit(uint8_t bitmap[], unsigned bit)
{
	bitmap[bit / 8] &= ~(1 << (bit % 8));
}

static inline int test_bit(uint8_t bitmap[], unsigned bit)
{
	return ((bitmap[bit / 8] & (1 << (bit % 8))) != 0);
}

static int8_t add_queue_element(le_uint16_t node, gnrc_waye_waiting_node_t ** queue, uint8_t queue_size)
{
	gnrc_waye_conn_table_entry_t *ptr = waye_conn_table_find_node(node);

	if (ptr == NULL)
	{
		return -1;
	}

	gnrc_waye_waiting_node_t *waiting_node = (gnrc_waye_waiting_node_t *) malloc(sizeof(gnrc_waye_waiting_node_t));
	if(waiting_node == NULL){
		return -2;
	}

	uint8_t frametype;
	if(random_uint32_range(0,1) % 2 == 0){
		frametype = GNRC_WAYE_FRAMETYPE_ACTUATOR_ORDER;
	} else {
		frametype = GNRC_WAYE_FRAMETYPE_DATA_POLL;
	}
	waiting_node->frame_type = frametype;
	waiting_node->node_sector = ptr->best_sector;
	waiting_node->node_addr = ptr->addr;
	waiting_node->next = NULL;

	waiting_node->actuator_order.parameters = 0;	
	uint8_t order = random_uint32_range(1,2);
	uint8_t actuator_addr = random_uint32_range(1,2);
	waiting_node->actuator_order.order = order << 4;
	waiting_node->actuator_order.order ^= actuator_addr;
	waiting_node->actuator_order.addr.u16 = ptr->addr.u16;
	waiting_node->actuator_order.order_id = order_id;

	if(*queue != NULL){
		while ((*queue)->next != NULL){
			*queue = (*queue)->next;
		}
		//DEBUG2("Queue NULL\n");	
	}
	*queue = waiting_node;
	/*
	(*queue)->actuator_order.parameters = 0;
	(*queue)->actuator_order.order = order << 4;
	(*queue)->actuator_order.order ^= actuator_addr;
	(*queue)->actuator_order.addr = ptr->addr;
	*/
	queue_size++;
	order_id++;

	return queue_size;	
}

static void * _thread_add_element_to_queue(void *arg){
	(void) arg;
	while (true){
		ztimer_sleep(ZTIMER_USEC,100000);
		unsigned nodes = waye_conn_table_nodes_available();
		uint32_t random_node = random_uint32_range(0, nodes);
		gnrc_waye_conn_table_entry_t *ptr = waye_conn_table_get_head();

		for(uint32_t i = 0 ; i < random_node ; i++) {
			ptr = ptr->next;
		}
		//ptr = waye_conn_table_get_head();
		//gnrc_waye_conn_table_entry_t *ptr2 = ptr->next;
		/*
		int8_t result = add_queue_element(ptr->addr, &emergency_queue, emergency_queue_size);
		DEBUG2("CP Emergency %02x, addr 1 %02x, addr 2 %02x\n",ptr->addr,emergency_queue->actuator_order.addr.u16,emergency_queue->next->actuator_order.addr.u16);
		if(result >= 0){
			emergency_queue_size = result;
			DEBUG2("[TEMPS] %u\n",emergency_queue_size);
		}
		result = add_queue_element(ptr->addr, &emergency_queue, emergency_queue_size);
		DEBUG2("CP Emergency %02x, addr 1 %02x, addr 2 %02x\n",ptr->addr,emergency_queue->actuator_order.addr.u16,emergency_queue->next->actuator_order.addr.u16);
		if(result >= 0){
			emergency_queue_size = result;
			DEBUG2("[TEMPS] %u\n",emergency_queue_size);
		}
		*/
		int8_t result = add_queue_element(ptr->addr, &standard_queue[ptr->best_sector], standard_queue_size[ptr->best_sector]);
		//DEBUG2("CP Fil de l'eau %04x, order %x, addresse %02x\n",ptr->addr,standard_queue[ptr->best_sector]->actuator_order.addr.u16,standard_queue[ptr->best_sector]->node_addr.u16);
		if(result >= 0){
			standard_queue_size[ptr->best_sector] = result;
			DEBUG2("Order %u => CP Fil de l'eau %02x\n", order_id - 1, ptr->addr.u16);
		}
		nb_event++;
	}
	return NULL;
}	

static void matrix_interleaver_decode(unsigned char enc_frame[], unsigned char dec_frame[], uint8_t nb_column)
{
	uint8_t mask = 0x80;
	uint8_t index = 0;
	for (int i = 0; i < 24; i++)
	{
		for (int j = 0; j < nb_column; j++)
		{
			if (enc_frame[index] & mask)
			{
				matrix[i][j] = 0x01;
			}
			else
			{
				matrix[i][j] = 0x00;
			}
			mask >>= 1;
			if (!mask)
			{
				mask = 0x80;
				index++;
			}
		}
	}
	mask = 0x80;
	index = 0;
	for (int i = 0; i < nb_column; i++)
	{
		for (int j = 0; j < 24; j++)
		{
			if (matrix[j][i])
			{
				dec_frame[index] |= mask;
			}
			mask >>= 1;
			if (!mask)
			{
				mask = 0x80;
				index++;
			}
		}
	}
}

static void matrix_linearization(unsigned char * line_matrix, uint8_t nb_column){
	uint8_t mask = 0x80;
	uint8_t index = 0;
	for(int i = 0 ; i < 24 ; i++){
		for(int j = 0 ; j < nb_column ; j++){
			if(matrix[i][j]){
				*(line_matrix + index) |= mask;
			}
			mask >>= 1;
			if(!mask){
				mask = 0x80;
				index++;
			}
		}
	}
}

static void put_error_in_frame(unsigned char * payload1, uint8_t pay_len1, float error_rate, uint8_t burst_size){
		if(error_rate > 1.0 ){ error_rate = 1.0 ; }
		uint16_t bit_nb = pay_len1 * 8;
		uint16_t bit_error_nb = floor(bit_nb * error_rate);
		unsigned char payload_mask[pay_len1];
		uint8_t mask = 0x00, index = 0;
		uint32_t pos = 0;
		//if (burst_size > bit_error_nb) { burst_size = bit_error_nb ; }
		if (burst_size > bit_error_nb) { DEBUG_PUTS2("-------------------------") ; return; }
		uint8_t original_burst_size = burst_size;
		uint32_t desync = 0;

		memset(payload_mask, 0, pay_len1);

		for(int i = 0 ; i < bit_error_nb ; ){
			pos = random_uint32_range(0, bit_nb-burst_size);
			//DEBUG2(" i %d pos %u |",i,pos);
			index = pos / 8;
			mask = 0x80 >> pos % 8;
			bool already_changed = false ;
			desync = random_uint32_range(0,1000);
			if(i > bit_error_nb - burst_size) { burst_size = bit_error_nb - i ; original_burst_size = burst_size ; }
			if(desync > 300){
				burst_size = burst_size+1;
			}

			for(int j = 0 ; j < burst_size ; j++){
				if(payload_mask[index] & mask){
					already_changed = true ;
				}
				mask >>= 1;
				if(!mask){
					mask = 0x80;
					index++;
				}
			}
			if(!already_changed){
				index = pos / 8;
				mask = 0x80 >> pos % 8;
				for(int j = 0 ; j < burst_size ; j++){
					payload_mask[index] |= mask;
					i++;

					mask >>= 1;
					if(!mask){
						mask = 0x80;
						index++;
					}
				}
			}
			burst_size = original_burst_size;
		}
		for(int i = 0 ; i < pay_len1 ; i++){
			payload1[i] ^= payload_mask[i];
		}
		//puts("");
}

void printu8(uint8_t d){
	uint8_t u, mask;
	mask = 1;
	mask <<= 7;
	for(u = 1; u<=8;u++){
		//DEBUG_PUTS2(((d & mask) ==0) ? '0' : '1' );

		putchar(((d & mask)==0) ? '0' : '1' );
		d <<= 1;
	}
	DEBUG2(" ");
}
