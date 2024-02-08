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
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include "thread.h"
#include "xtimer.h"
#include "net/gnrc.h"
#include "net/gnrc/netreg.h"
#include "net/gnrc/mac/internal.h"
#include "net/gnrc/waye/waye.h"
#include "shell.h"
#include "led.h"
#include "net/gnrc/netif/ieee802154.h"
#include "net/gnrc/netif/mac.h"
#include "net/netdev/ieee802154.h"
#include "random.h"
#include "ecc/golay2412.h"
#include "ecc/interleaver.h"
#include "ecc/repetition.h"
#include <signal.h>
#include <unistd.h>
#include "periph/rtc.h"
#include "at86rf215_internal.h"
#include "at86rf215.h"
#include "at86rf215_registers.h"
#include "ztimer.h"
#include "periph/rtt.h"
#include "raw_send.h"
#include "include/waye_internal.h"

#define ENABLE_DEBUG 1
#include "debug.h"

#define GNRC_WAYE_EVENT_RECV 	1
#define GNRC_WAYE_EVENT_SLOT_COLLECT	2
#define GNRC_WAYE_EVENT_SLOT_DISCOVERY	3

#define UNUSED(x) (void)(x)

//#define _BSD_SOURCE

#define TOS_AM_GROUP (6)
#define Q_SZ (8)

/**
 * @brief Global variable Golay error corrected
 */
volatile bool waye_golay_error_corrected = true;

static int _waye_hello_init(gnrc_netif_t *netif);
static void _waye_hello_msg_handler(gnrc_netif_t *netif, msg_t *msg);
static int _send(gnrc_netif_t *netif, gnrc_pktsnip_t *pkt);
static gnrc_pktsnip_t *_recv(gnrc_netif_t *netif);
static void _init_from_device(gnrc_netif_t *netif);
static void waye_neighbor_discovery_cb(void *arg);
static void waye_neighbor_discovery_handler(gnrc_netif_t *netif);
static void waye_collect_slot_end_cb(void *arg);
static void waye_collect_slot_end_handler(gnrc_netif_t *netif);
static void waye_collect_send_cb(void *arg);
static void waye_collect_send_handler(gnrc_netif_t *netif);
//static void waye_collect_slot_end_handler(void);
static void * _waye_thread_data_collect(void *arg);
static void put_in_matrix(unsigned char  sample_golay_words[6], uint8_t position);
static void matrix_linearization(unsigned char * line_matrix, uint8_t nb_column);
static void put_error_in_frame(unsigned char * payload, uint8_t pay_len, float error_rate, uint8_t burst_size);
void printu8(uint8_t d);

static const gnrc_netif_ops_t waye_ops = {
	.init = _waye_hello_init,
	.recv = _recv,
	.send = _send,
	.msg_handler = _waye_hello_msg_handler,
};


char recv_thread_stack[THREAD_STACKSIZE_MAIN];
char send_thread_stack[THREAD_STACKSIZE_MAIN];
char data_thread_stack[THREAD_STACKSIZE_MAIN];
static gnrc_waye_frame_hello_t hello_frame;
static gnrc_waye_frame_data_reply_t data_reply_frame;
static gnrc_waye_frame_data_golayed_t data_golayed_frame;

uint8_t sink_addr[2];
uint8_t header[18];
static le_uint16_t neighbor[GNRC_WAYE_HELLO_QUEUE_SIZE];
static uint8_t nb_detected_neighbor = 0;
size_t length_mhr = 0;

static bool collection_phase = false;
static bool thread_created = false;
static bool ack_received = false;

static uint32_t start_recv_counter;
static uint32_t save_start_recv_counter = 0;
static uint32_t end_config;
static uint32_t rx_start;
static uint32_t save_rx_start = 0;

static uint32_t tx_complete = 0;

static uint32_t start_tx = 0;
static uint32_t tx_start_counter = 0;
static uint32_t endbuffer = 0;

static uint8_t node_type;

static uint8_t crc_state = 1;
static uint8_t combien_rx = 0;


static gnrc_waye_data_queue_element_t data[GNRC_WAYE_DATA_REPLY_QUEUE_SIZE];
//static gnrc_waye_data_golayed_element_t data_golayed[GNRC_WAYE_DATA_REPLY_QUEUE_SIZE];
static unsigned char golay_header1[3];
static unsigned char golay_header1_extra_slot[6];
static unsigned char golay_header2[3];
static uint8_t data_index = 0;
static uint8_t data_len = 1;
static uint16_t frame_length = 0;
static uint32_t start_slots;
static uint32_t wait_slot;
static uint32_t wait_ack_slot;
static uint32_t wait_extra_slot;
static uint32_t wait_ack_extra_slot;
static bool use_extra_slot = false;
static uint8_t collect_nb_node;

static uint16_t nb_reception = 0;
static uint32_t recv_ack = 0;

static uint8_t burst = 72;
static float error = 0.00;
static uint32_t nb_poll = 1;

static unsigned char matrix[24][24];


int gnrc_netif_waye_create(gnrc_netif_t *netif,char *stack, int stacksize, char priority, const char *name, netdev_t *dev) {

	return gnrc_netif_create(netif, stack, stacksize, priority, name, dev, &waye_ops);
}

static void _waye_hello_event_cb(netdev_t *dev, netdev_event_t event) {
	gnrc_netif_t *netif = (gnrc_netif_t *) dev->context;

	if (event == NETDEV_EVENT_ISR) {
		msg_t msg = { .type = NETDEV_MSG_TYPE_EVENT,
                          .content = { .ptr = netif } };
				  
		if (msg_send(&msg, netif->pid) <= 0) {
			printf("WARNING: [WAYE] gnrc_netdev: possibly lost interrupt.\n");
		}
	}
	else {
		//printf("gnrc_netdev: event triggered -> %i\n", event);

		switch (event) {
			case NETDEV_EVENT_RX_STARTED:
				//DEBUG("[WAYE] NETDEV_EVENT_RX_STARTED\n");
				rx_start = rtt_get_counter();
				gnrc_netif_set_rx_started(netif, true);
				break;

			case NETDEV_EVENT_RX_COMPLETE:
				//DEBUG("[WAYE] NETDEV_EVENT_RX_COMPLETE\n");
				start_recv_counter = rtt_get_counter();
				gnrc_netif_set_rx_started(netif, false);
				_recv(netif);
				break;

			case NETDEV_EVENT_TX_STARTED:
				//DEBUG("[WAYE] NETDEV_EVENT_TX_STARTED\n");
				tx_start_counter = rtt_get_counter();
				gnrc_netif_set_tx_feedback(netif, TX_FEEDBACK_UNDEF);
				gnrc_netif_set_rx_started(netif, false);
				break;

			case NETDEV_EVENT_TX_COMPLETE:
				//DEBUG("[WAYE] NETDEV_EVENT_TX_COMPLETE, frame length = %u\n",frame_length);
				ack_received = true;
				tx_complete = rtt_get_counter();
				//DEBUG("Temps tx_complete : %lu\n",TICKS_TO_US(tx_complete-start_tx));
				gnrc_netif_set_tx_feedback(netif, TX_FEEDBACK_SUCCESS);
				gnrc_netif_set_rx_started(netif, false);
				
				break;

			case NETDEV_EVENT_TX_NOACK:
				//DEBUG("[WAYE] NETDEV_EVENT_TX_NOACK\n");
				ack_received = false;
				gnrc_netif_set_tx_feedback(netif, TX_FEEDBACK_NOACK);
				gnrc_netif_set_rx_started(netif, false);
				break;

			case NETDEV_EVENT_TX_MEDIUM_BUSY:
				printf("[WAYE] NETDEV_EVENT_TX_MEDIUM_BUSY\n");
				
				gnrc_netif_set_tx_feedback(netif, TX_FEEDBACK_BUSY);
				gnrc_netif_set_rx_started(netif, false);
				break;

			default:
				printf("WARNING [WAYE]: Unhandled event %u.\n", event);
		}
	}
}

static void _init_from_device(gnrc_netif_t *netif)
{
    int res;
    netdev_t *dev = netif->dev;
    uint16_t tmp;

    res = dev->driver->get(dev, NETOPT_DEVICE_TYPE, &tmp, sizeof(tmp));
    (void)res;
    assert(res == sizeof(tmp));
    netif->device_type = (uint8_t)tmp;
}

static int _waye_hello_init(gnrc_netif_t *netif) {
	netdev_t *dev = netif->dev;
	uint16_t saddr;

	
	dev->event_callback = _waye_hello_event_cb;
	dev->context = netif;
	hello_frame.sector = 1;

	if(dev->driver->init(dev) < 0 ){
		DEBUG("[WAYE]PROBLEM DRIVER INIT\n");
	}
	netif_register(&netif->netif);
    _init_from_device(netif);

	rtt_init();

	/* Set the address sent to the node short address */
	dev->driver->get(dev,NETOPT_ADDRESS,&saddr,sizeof(saddr));
	hello_frame.addr.u16 = saddr;
	DEBUG("sizeof addr 8 bit : %u\n",sizeof(hello_frame.addr.u8));

	/* Enable RX_START and TX_END interrupts */
	netopt_enable_t enable = NETOPT_ENABLE;
	dev->driver->set(dev, NETOPT_RX_START_IRQ, &enable, sizeof(enable));
	dev->driver->set(dev, NETOPT_TX_END_IRQ, &enable, sizeof(enable));
	//dev->driver->set(dev, NETOPT_PROMISCUOUSMODE, &enable, sizeof(enable));

	/* Doesn't work, not defined in driver. Forced inside waye_internal.c, with the flags variable. */
	dev->driver->set(dev, NETOPT_ACK_REQ, &enable, sizeof(enable));


	/* Disable CSMA/CA */
	enable = NETOPT_DISABLE;
	dev->driver->set(dev, NETOPT_CSMA, &enable, sizeof(enable));

	//datain->data = { 201, 240, 154,   5, 227,  60, 116, 192, 214, 240, 154,   5, 227,  60, 116, 192, 214, 240, 154,   5 };
	netdev_ieee802154_t *device_state = container_of(dev,netdev_ieee802154_t, netdev);
    at86rf215_t * at86rf215_dev = container_of(device_state, at86rf215_t, netdev);

	DEBUG("My get addr short : %x\n",at86rf215_get_addr_short(at86rf215_dev,0));
	
	uint32_t seed;
	at86rf215_get_random(at86rf215_dev, (uint8_t *)&seed, sizeof(seed));
	random_init(seed);
	DEBUG("ACK TIMEOUT : %lu\n",at86rf215_dev->ack_timeout_usec);
	at86rf215_dev->retries_max = 0;
	at86rf215_dev->retries = 0;
	rtt_set_counter(0);

/**
 * node_type est une variable qui permet de savoir le type du noeud
 * if = 0 => le noeud est un actionneur
 * if = 1 => le noeud est un capteur
 * if = 2 => le noeud est un capteur actionneur
*/
	node_type = random_uint32_range(2,3);
	DEBUG("Type du noeud : %u \n",node_type);
/*
	uint8_t reg_pc = at86rf215_reg_read(at86rf215_dev,at86rf215_dev->BBC->RG_PC);
	reg_pc ^= PC_TXAFCS_MASK;
	DEBUG("CRC ON : %x, crc state : %d\n",reg_pc, (reg_pc & PC_TXAFCS_MASK));
	at86rf215_reg_write(at86rf215_dev,at86rf215_dev->BBC->RG_PC,reg_pc);
	if((reg_pc & PC_TXAFCS_MASK) == 0) {
		DEBUG("CRC DESACTIVE\n");
		crc_state = 0;
	}
	*/

	for(int i = 0 ; i < 24 ; i++){
		memset(matrix[i],0,24);
	}

	return 0;

}
static gnrc_pktsnip_t *_make_netif_hdr(uint8_t *mhr) {
	gnrc_pktsnip_t *snip;
	uint8_t src[IEEE802154_LONG_ADDRESS_LEN], dst[IEEE802154_LONG_ADDRESS_LEN];
	int src_len, dst_len;
	le_uint16_t _pan_tmp;   /* TODO: hand-up PAN IDs to GNRC? */
	for(unsigned i = 0 ; i < length_mhr ; i++){
			header[i] = mhr[i];
	}
	if(length_mhr == 9) {
		collection_phase = true;
/*		
		for(unsigned i = 0 ; i < 9 ; i++){
			header[i] = mhr[i];
		}
*/		
		sink_addr[0]=mhr[7];
		sink_addr[1]=mhr[8];
	}
/*	
	if(collection_phase){
		DEBUG("%x %x %x %x \n",mhr[5],mhr[6],mhr[7],mhr[8]);
	}
*/	
	dst_len = ieee802154_get_dst(mhr, dst, &_pan_tmp);
	src_len = ieee802154_get_src(mhr, src, &_pan_tmp);
	if ((dst_len < 0) || (src_len < 0)) {
		
		return NULL;
	}
	/* allocate space for header */
	snip = gnrc_netif_hdr_build(src, (size_t)src_len, dst, (size_t)dst_len);
	if (snip == NULL) {
		
		return NULL;
	}
	/* set broadcast flag for broadcast destination */
	if ((dst_len == 2) && (dst[0] == 0xff) && (dst[1] == 0xff)) {
		gnrc_netif_hdr_t *hdr = snip->data;
		hdr->flags |= GNRC_NETIF_HDR_FLAGS_BROADCAST;
	}
	return snip;
}
static int _send(gnrc_netif_t *netif, gnrc_pktsnip_t *pkt) {
	//printf("sending back %d\n",(int)pkt->size);
	return _gnrc_waye_transmit(netif, pkt);
}
static gnrc_pktsnip_t *_recv(gnrc_netif_t *netif) {
	netdev_t *dev = netif->dev;
	netdev_ieee802154_rx_info_t rx_info;
	netdev_ieee802154_t *state = container_of(dev, netdev_ieee802154_t, netdev);
	gnrc_pktsnip_t *pkt = NULL;

	int bytes_expected = dev->driver->recv(dev, NULL, 0, NULL);

	if (bytes_expected >= (int)IEEE802154_MIN_FRAME_LEN) {
		int nread;

		pkt = gnrc_pktbuf_add(NULL, NULL, bytes_expected, GNRC_NETTYPE_UNDEF);
		if (pkt == NULL) {
			
			/* Discard packet on netdev device */
			dev->driver->recv(dev, NULL, bytes_expected, NULL);
			return NULL;
		}
		nread = dev->driver->recv(dev, pkt->data, bytes_expected, &rx_info);
		if (nread <= 0) {
			gnrc_pktbuf_release(pkt);
			return NULL;
		}
		if (state->flags & GNRC_NETIF_FLAGS_RAWMODE) {
			/* Raw mode, skip packet processing, but provide rx_info via
			 * GNRC_NETTYPE_NETIF */
			gnrc_pktsnip_t *netif_snip = gnrc_netif_hdr_build(NULL, 0, NULL, 0);
			if (netif_snip == NULL) {
				
				gnrc_pktbuf_release(pkt);
				return NULL;
			}
			gnrc_netif_hdr_t *hdr = netif_snip->data;
			hdr->lqi = rx_info.lqi;
			hdr->rssi = rx_info.rssi;
			hdr->if_pid = netif->pid;
			LL_APPEND(pkt, netif_snip);
		}
		else {
			/* Normal mode, try to parse the frame according to IEEE 802.15.4 */
			gnrc_pktsnip_t *ieee802154_hdr, *netif_hdr;
			gnrc_netif_hdr_t *hdr;

			size_t mhr_len = ieee802154_get_frame_hdr_len(pkt->data);

			if (mhr_len == 0) {
				
				gnrc_pktbuf_release(pkt);
				return NULL;
			}
			frame_length = nread;
			nread -= mhr_len;
			/* mark IEEE 802.15.4 header */
			ieee802154_hdr = gnrc_pktbuf_mark(pkt, mhr_len, GNRC_NETTYPE_UNDEF);
			if (ieee802154_hdr == NULL) {
				
				gnrc_pktbuf_release(pkt);
				return NULL;
			}
			length_mhr = mhr_len;
			netif_hdr = _make_netif_hdr(ieee802154_hdr->data);
			if (netif_hdr == NULL) {
				
				gnrc_pktbuf_release(pkt);
				return NULL;
			}

			hdr = netif_hdr->data;

#ifdef MODULE_L2FILTER
			if (!l2filter_pass(dev->filter, gnrc_netif_hdr_get_src_addr(hdr),
							   hdr->src_l2addr_len)) {
				gnrc_pktbuf_release(pkt);
				gnrc_pktbuf_release(netif_hdr);
				
				return NULL;
			}
#endif

			hdr->lqi = rx_info.lqi;
			hdr->rssi = rx_info.rssi;
			hdr->if_pid = thread_getpid();
			dev->driver->get(dev, NETOPT_PROTO, &pkt->type, sizeof(pkt->type));

#if defined(MODULE_OD)
			od_hex_dump(pkt->data, nread, OD_WIDTH_DEFAULT);

#endif
			gnrc_pktbuf_remove_snip(pkt, ieee802154_hdr);
			LL_APPEND(pkt, netif_hdr);
		}

		
		gnrc_pktbuf_realloc_data(pkt, nread);
	} else if (bytes_expected > 0) {
		
		dev->driver->recv(dev, NULL, bytes_expected, NULL);
	}

	uint8_t *ptr2 = pkt->data;

	bool foundit = false;

	DEBUG("value of ptr : type %u \n", *ptr2 );

	if (*ptr2 == GNRC_WAYE_FRAMETYPE_WAYE){
		gnrc_waye_frame_waye_t *hello2 = (gnrc_waye_frame_waye_t *)(ptr2 + 1);
		collection_phase = false;

			hello_frame.discovery_id = hello2->discovery_id;
			hello_frame.sector = hello2->sector;
			hello_frame.node_type = node_type;
			//hello_frame.sector = 2;
			hello_frame.round = hello2->round;

			for (uint8_t j = 11; j < pkt->size + 2; j=j+2){
				le_uint16_t *bla = (le_uint16_t*)(ptr2 + j);
				if(bla->u16==hello_frame.addr.u16){
					foundit = true;
				}
			}
			if(!foundit){
				//srand(time(NULL));
				if(!thread_created){
					thread_create(data_thread_stack, sizeof(data_thread_stack), THREAD_PRIORITY_MAIN - 6, THREAD_CREATE_STACKTEST, _waye_thread_data_collect, NULL, "data-thread");
				}
				thread_created = true;

				//hello_frame.selected_slot = 0;
				//hello_frame.selected_slot = rand() % (hello2->nb_slot); //hello_frame.selected_slot;//(random avec nb.slot comme interval)
				hello_frame.selected_slot = random_uint32_range(0, hello2->nb_slot);
				//printf("sending at %d\n",hello_frame.selected_slot);				
				//xtimer_usleep((hello_frame.selected_slot*hello2->slot_size_ms*1000)+1700);

				
				ztimer_sleep(ZTIMER_USEC,(hello_frame.selected_slot*hello2->slot_size_ms*1000)+1700);
				start_tx = rtt_get_counter();
				gnrc_waye_send_hellos(netif, (gnrc_waye_frame_hello_t *)&hello_frame);
				//hello_frame.selected_slot++;
				printf("ENVOYE !!!\n");

				rtt_set_alarm(RTT_US_TO_TICKS((hello_frame.selected_slot*hello2->slot_size_ms*1000)+1700), waye_neighbor_discovery_cb,netif);

			}
	}
	
	else if(*ptr2 == GNRC_WAYE_FRAMETYPE_DATA_POLL)
	{
		combien_rx++;
		save_rx_start = rx_start;
		save_start_recv_counter = start_recv_counter;
		start_slots = start_recv_counter + RTT_US_TO_TICKS(1700);
		rtt_set_alarm(start_slots,waye_collect_send_cb,netif);
		
		gnrc_waye_frame_data_poll_t *hello3 = (gnrc_waye_frame_data_poll_t *)(ptr2 + 1);
		collect_nb_node = 0;
		ack_received = false;
		use_extra_slot = false;
	
		for (uint8_t j = 3; j < pkt->size; j=j+2){
				//printf("pkt->size = %d\n",pkt->size);
				//foundit2=false;
				le_uint16_t *blabla = (le_uint16_t*)(ptr2 + j);
				collect_nb_node++;
				//printf("L ADDRESSE ! 0x%04x\n",blabla->u16);//bla->u16
				//printf("Mon addresse est : 0x%04x, 8 bits : 0x%x %x\n",hello_frame.addr.u16,hello_frame.addr.u8[0],hello_frame.addr.u8[1]);
				if(blabla->u16==hello_frame.addr.u16){
					data_reply_frame.selected_slot = ((j-3) / 2);
					foundit = true;
				}
		}
		if(foundit){
			nb_poll++;

			data_reply_frame.src_addr[0] = hello_frame.addr.u8[0];
			data_reply_frame.src_addr[1] = hello_frame.addr.u8[1];
			data_reply_frame.poll_id = hello3->poll_id;

			netdev_ieee802154_t *state = container_of(dev, netdev_ieee802154_t, netdev);
			// -1 so that it cancels itself with the +1 when sending in gnrc_waye_send_data_reply
			data_reply_frame.seq_num = state->seq;

			golay_header1[0] = data_reply_frame.seq_num;
			golay_header1[1] = data_reply_frame.src_addr[0];
			golay_header1[2] = data_reply_frame.src_addr[1];		



			golay2412_encode(3,golay_header1,data_golayed_frame.data_queue[0].data_golayed);
			put_in_matrix(data_golayed_frame.data_queue[0].data_golayed,0);

			golay_header1[0] = data_reply_frame.seq_num ;
			golay2412_encode(3,golay_header1,golay_header1_extra_slot);

			if(data_len < GNRC_WAYE_DATA_REPLY_QUEUE_SIZE) {
				data_reply_frame.data_queue_len = data_len;
			} else {
				data_reply_frame.data_queue_len = GNRC_WAYE_DATA_REPLY_QUEUE_SIZE;
			}

			golay_header2[0] = data_reply_frame.poll_id;
			golay_header2[1] = data_reply_frame.selected_slot;
			golay_header2[2] = data_reply_frame.data_queue_len;


			golay2412_encode(3,golay_header2,data_golayed_frame.data_queue[1].data_golayed);
			put_in_matrix(data_golayed_frame.data_queue[1].data_golayed,1);

			wait_slot = start_slots + RTT_US_TO_TICKS(hello3->slot_size_ms*data_reply_frame.selected_slot*1000);
			wait_ack_slot = wait_slot + RTT_US_TO_TICKS(hello3->slot_size_ms*1000);
			wait_extra_slot = wait_slot + RTT_US_TO_TICKS(hello3->slot_size_ms*1000 * (collect_nb_node - data_reply_frame.selected_slot));
			wait_ack_extra_slot = wait_ack_slot + RTT_US_TO_TICKS(hello3->slot_size_ms*1000 * (collect_nb_node - data_reply_frame.selected_slot));
			end_config = rtt_get_counter();

			//LED1_TOGGLE;
		}
		else{
			rtt_clear_alarm();
		}
	}
	else if(*ptr2 == GNRC_WAYE_FRAMETYPE_DATA_ACK)
	{
		nb_reception++;

		ack_received = true;
		recv_ack = rtt_get_counter();

		//printf("RECEIVED AN ACK FOR MY MESSAGE !!!\n");
		
	}
	else if(*ptr2 == GNRC_WAYE_FRAMETYPE_MIXED_POLL)
	{
		size_t j = 2;

		combien_rx++;
		save_rx_start = rx_start;
		save_start_recv_counter = start_recv_counter;
		start_slots = start_recv_counter + RTT_US_TO_TICKS(1700);
		rtt_set_alarm(start_slots,waye_collect_send_cb,netif);
		gnrc_waye_frame_data_mixed_poll_t *hello4 = (gnrc_waye_frame_data_mixed_poll_t *)(ptr2 + 1);
		collect_nb_node = 0;
		ack_received = false;
		use_extra_slot = false;

		if(node_type != GNRC_WAYE_NODE_TYPE_ACTUATOR){
			for(uint8_t i = 0; i < *(ptr2+3) ; i++ ){
				le_uint16_t *compare_addr = (le_uint16_t*)(ptr2 + 4 + (j * i));
				if(compare_addr->u16 == hello_frame.addr.u16){
					foundit = true;
				}
			}
			if(foundit){
				nb_poll++;

				data_reply_frame.src_addr[0] = hello_frame.addr.u8[0];
				data_reply_frame.src_addr[1] = hello_frame.addr.u8[1];
				data_reply_frame.poll_id = hello4->poll_id;

				netdev_ieee802154_t *state = container_of(dev, netdev_ieee802154_t, netdev);
				// -1 so that it cancels itself with the +1 when sending in gnrc_waye_send_data_reply
				data_reply_frame.seq_num = state->seq;

				golay_header1[0] = data_reply_frame.seq_num;
				golay_header1[1] = data_reply_frame.src_addr[0];
				golay_header1[2] = data_reply_frame.src_addr[1];		



				golay2412_encode(3,golay_header1,data_golayed_frame.data_queue[0].data_golayed);
				put_in_matrix(data_golayed_frame.data_queue[0].data_golayed,0);

				golay_header1[0] = data_reply_frame.seq_num ;
				golay2412_encode(3,golay_header1,golay_header1_extra_slot);

				if(data_len < GNRC_WAYE_DATA_REPLY_QUEUE_SIZE) {
					data_reply_frame.data_queue_len = data_len;
				} else {
					data_reply_frame.data_queue_len = GNRC_WAYE_DATA_REPLY_QUEUE_SIZE;
				}

				golay_header2[0] = data_reply_frame.poll_id;
				golay_header2[1] = data_reply_frame.selected_slot;
				golay_header2[2] = data_reply_frame.data_queue_len;


				golay2412_encode(3,golay_header2,data_golayed_frame.data_queue[1].data_golayed);
				put_in_matrix(data_golayed_frame.data_queue[1].data_golayed,1);

				wait_slot = start_slots + RTT_US_TO_TICKS(hello4->slot_size_ms*data_reply_frame.selected_slot*1000);
				wait_ack_slot = wait_slot + RTT_US_TO_TICKS(hello4->slot_size_ms*1000);
				wait_extra_slot = wait_slot + RTT_US_TO_TICKS(hello4->slot_size_ms*1000 * (collect_nb_node - data_reply_frame.selected_slot));
				wait_ack_extra_slot = wait_ack_slot + RTT_US_TO_TICKS(hello4->slot_size_ms*1000 * (collect_nb_node - data_reply_frame.selected_slot));
				end_config = rtt_get_counter();
				

				//LED1_TOGGLE;
			}
		}
		foundit = false;
		if(node_type != GNRC_WAYE_NODE_TYPE_CAPTOR){
			uint8_t taille_trame = 4 + hello4->list_poll_size * 2;
			uint8_t pos_found = 0;

			for(uint8_t i = taille_trame; i < pkt->size ; i = i + 7 ){
				le_uint16_t *compare_addr = (le_uint16_t*)(ptr2 + i);
				//DEBUG("[mixed] %02x \n",compare_addr->u16);
				if(compare_addr->u16 == hello_frame.addr.u16){
					foundit = true;
					pos_found = i;
					gnrc_waye_frame_actuator_order_t * ordered2_frame = (gnrc_waye_frame_actuator_order_t *)(ptr2 + pos_found);

					uint8_t actuator = ordered2_frame->order & 0x0f;
					uint8_t order = ordered2_frame->order & 0xf0;
					DEBUG("Order %u => [mixed] %02x \n", ordered2_frame->order_id, hello_frame.addr.u16);
					switch(actuator) {
						case 0x01 : 
							switch(order) {
								case 0x00 : 
									LED0_TOGGLE;
									puts("led0");
									break;
								case 0x10 :
									LED1_TOGGLE;
									puts("led1");
									break;
								case 0x20 :
									LED2_TOGGLE;
									puts("led2");
									break;
								case 0x30 :
									LED3_TOGGLE;
									puts("led3");
									break;
								default :
									puts("no led");
									DEBUG("ORDER : %x \n",order);
									break;
							}
							break;
						case 0x02 :
							switch(order) {
								case 0x00 :
									puts("Actionneur 2 sollicité pour le cas 1");
									break;
								case 0x10 :
									puts("Actionneur 2 sollicité pour le cas 2");
									break;
								default :
									puts("Actionneur 2 sans ordre");
									break;
							}
							break;
						default :
							puts("Probleme de reception d'ordre");
							break;
					}
				}
			}
		}
	}
	else if(*ptr2 == GNRC_WAYE_FRAMETYPE_ACTUATOR_ORDER)
	{
		for(uint8_t i = 1; i < pkt->size ; i = i + 7 ){
			le_uint16_t *compare_addr = (le_uint16_t*)(ptr2 + i);
			if(compare_addr->u16 == hello_frame.addr.u16){
				foundit = true;
				gnrc_waye_frame_actuator_order_t * ordered2_frame = (gnrc_waye_frame_actuator_order_t *)(ptr2+i);
				uint8_t actuator = ordered2_frame->order & 0x0f;
				uint8_t order = ordered2_frame->order & 0xf0;
				DEBUG("Order %u => [order] %02x \n",ordered2_frame->order_id,hello_frame.addr.u16);
				switch(actuator) {
					case 0x01 : 
						switch(order) {
							case 0x00 : 
								LED0_TOGGLE;
								puts("led0");
								break;
							case 0x10 :
								LED1_TOGGLE;
								puts("led1");
								break;
							case 0x20 :
								LED2_TOGGLE;
								puts("led2");
								break;
							case 0x30 :
								LED3_TOGGLE;
								puts("led3");
								break;
							default :
								puts("no led");
								DEBUG("ORDER : %x \n",order);
								break;
						}
						break;
					case 0x02 :
						switch(order) {
							case 0x00 :
								puts("Actionneur 2 sollicité pour le cas 1");
								break;
							case 0x10 :
								puts("Actionneur 2 sollicité pour le cas 2");
								break;
							default :
								puts("Actionneur 2 sans ordre");
								break;
						}
						break;
					default :
						puts("Probleme de reception d'ordre");
						break;
				}
			}
		}
	}

	else if(*ptr2 == GNRC_WAYE_FRAMETYPE_HELLO)
	{
		gnrc_waye_frame_hello_t *hello_neigh = (gnrc_waye_frame_hello_t *)(ptr2 + 1);
		neighbor[nb_detected_neighbor] = hello_neigh->addr;
		nb_detected_neighbor++;
	}
	gnrc_pktbuf_release(pkt);
	return pkt;
}
/*
static void swap_first_and_fiftiest_bytes(unsigned char * samples){
	unsigned char tmp0;
	unsigned char tmp1;
	uint8_t oct[8]={8,13,15,36,44,47,51,64};
	uint8_t decal = 0x01;
	uint8_t count = 0;
	uint8_t ind = 0;
	do {
		for(int q = 0 ; q < 8 ; q++){
			tmp0 = samples[oct[q] + count] & decal;
			tmp1 = samples[ind] & decal;
			samples[ind] = tmp0 | (samples[ind] & ~(decal));
			samples[oct[q] + count] = tmp1 | (samples[oct[q] + count] & ~(decal));
			decal <<= 1;
		}
		count++;
		ind = 49;
		decal = 0x01;
	} while(count < 2);
}
*/

static void waye_neighbor_discovery_cb(void *arg){
	msg_t msg = { .type = GNRC_WAYE_EVENT_SLOT_DISCOVERY, .content = { .ptr = arg } };
	gnrc_netif_t *netif = (gnrc_netif_t *)arg;

	msg_send(&msg, netif->pid);

	if (sched_context_switch_request) {
		thread_yield();
	}
}
static void waye_collect_send_cb(void *arg) {
	msg_t msg = { .type = GNRC_WAYE_EVENT_RECV, .content = { .ptr = arg } };
	gnrc_netif_t *netif = (gnrc_netif_t *)arg;

	msg_send(&msg, netif->pid);

	if (sched_context_switch_request) {
		thread_yield();
	}
}

static void waye_collect_slot_end_cb(void *arg) {
	msg_t msg = { .type = GNRC_WAYE_EVENT_SLOT_COLLECT, .content = { .ptr = arg } };
	gnrc_netif_t *netif = (gnrc_netif_t *)arg;

	msg_send(&msg, netif->pid);

	if (sched_context_switch_request) {
		thread_yield();
	}
}

static void waye_neighbor_discovery_handler(gnrc_netif_t *netif){

	if(!nb_detected_neighbor){
		gnrc_waye_send_hellos(netif, (gnrc_waye_frame_hello_t *)&hello_frame);
	} else {
		gnrc_waye_send_hellos_coop(netif, (gnrc_waye_frame_hello_t *)&hello_frame,neighbor,nb_detected_neighbor);
	}
	
	memset(neighbor,0,sizeof(le_uint16_t)*GNRC_WAYE_HELLO_QUEUE_SIZE);
	nb_detected_neighbor = 0;

}

static void waye_collect_send_handler(gnrc_netif_t *netif){
	//netdev_t* dev = netif->dev;
	if(wait_slot > rtt_get_counter()){

		rtt_set_alarm(wait_slot,waye_collect_send_cb,netif);
	} else {
		size_t payload_len = 12 + 6 * data_reply_frame.data_queue_len;

		uint8_t nb_column = payload_len/3;

		unsigned char reply_frame[payload_len];
		memset(reply_frame,0,payload_len);

		matrix_linearization(reply_frame,nb_column);

		if(crc_state == 0) {
			uint32_t err = random_uint32_range(1,13);
			error = (float) err / 100.0;
			put_error_in_frame(reply_frame,payload_len,error,burst);
		}

		start_tx = rtt_get_counter();

		//gnrc_waye_send_data_golay(netif, (gnrc_waye_frame_data_golayed_t *)&data_reply_interleaver, sink_addr, hello_frame.addr.u8, crc_state);
		//gnrc_waye_send_data_golay(netif, data_reply_interleaver, sink_addr, hello_frame.addr.u8, crc_state, data_size);
		//gnrc_waye_send_data_golay(netif, data_reply_interleaver1, sink_addr, hello_frame.addr.u8, crc_state, payload_len);
		gnrc_waye_send_data_golay(netif, reply_frame, sink_addr, hello_frame.addr.u8, crc_state, payload_len);

		endbuffer = rtt_get_counter();
		
		//at86rf215_t * netdev = container_of(state,at86rf215_t,netdev );
		//at86rf215_filter_ack(netdev,true);
		rtt_set_alarm(wait_ack_slot,waye_collect_slot_end_cb,netif);
	}
}



static void waye_collect_slot_end_handler(gnrc_netif_t *netif) {
	netdev_t* dev = netif->dev;
	netdev_ieee802154_t *state = container_of(dev, netdev_ieee802154_t, netdev);
	at86rf215_t * at86rf215_dev = container_of(state,at86rf215_t,netdev);
	//printf("thread %s\n",thread_getname(netif->pid));
	if(ack_received || use_extra_slot){
		uint8_t reg_pc = at86rf215_reg_read(at86rf215_dev,at86rf215_dev->BBC->RG_PC);
		
		reg_pc &= PC_TXAFCS_MASK;
		

		at86rf215_reg_write(at86rf215_dev,at86rf215_dev->BBC->RG_PC,reg_pc);
		
		if((reg_pc & PC_TXAFCS_MASK) == 0) {
			//DEBUG("CRC DESACTIVE\n");
			crc_state = 0;
		} 
		if((reg_pc & PC_TXAFCS_MASK) == 0x10) {
			//DEBUG("CRC ACTIVE\n");
			crc_state = 1;
		}
	}


	if(ack_received){
		//DEBUG("Ack bien reçu ! (seq_num = %u, nb_reception = %u, temps traitement = %lu)\n",data_reply_frame.seq_num,nb_reception, TICKS_TO_US(recv_ack-wait_slot));
		//DEBUG("Config timing : %lu, Recv Timing : %lu, RX_START to Endconfig : %lu\n",TICKS_TO_US(end_config-save_start_recv_counter),
		//			TICKS_TO_US(save_start_recv_counter-save_rx_start),	TICKS_TO_US(end_config-save_rx_start));
		//DEBUG("Transaction correcte : %lu , Data length : %u\n", TICKS_TO_US(tx_complete-wait_slot),data_reply_frame.data_queue_len);
		//DEBUG("Temps fonction send : %lu, TEMPS TX : %lu\n", TICKS_TO_US(tx_start_counter-wait_slot),TICKS_TO_US(tx_complete-tx_start_counter));
		//DEBUG("Temps rx end -> tx _start : %lu \n", TICKS_TO_US(tx_start_counter-start_recv_counter));
		//DEBUG("Rx_END-> ack : %lu \n",TICKS_TO_US(recv_ack - start_recv_counter));
		//DEBUG("GOLAY HEADER : %u %u %u %u %u %u\n\n",golay_header1[0],golay_header1[1],golay_header1[2],golay_header2[0],golay_header2[1],golay_header2[2]);
		memset(&data,0,sizeof(data));
		//data_index = 0;
		//data_len = 0;
		
		for(int i = 0 ; i < 24 ; i++){
			memset(matrix[i],0,24);
		}

		if(use_extra_slot){
			//DEBUG("en utilisant l'extra slot\n");
		}
	}
	else if(use_extra_slot){
		//DEBUG("Echec de l'extra slot ! seq_num = %u temps traitement = %lu, ws = %lu, was = %lu\n",data_reply_frame.seq_num,TICKS_TO_US(tx_complete-wait_slot),TICKS_TO_US(wait_extra_slot-wait_slot),TICKS_TO_US(wait_ack_extra_slot-wait_slot));
		return;
	}

	
	else {
		//DEBUG("Ack non reçu ! Renvoi du message \n");
		if(wait_extra_slot > rtt_get_counter()){
			// wait until extra slot
			
			rtt_set_alarm(wait_extra_slot,waye_collect_slot_end_cb,netif);
		} else {
			// resend in the extra slot
			use_extra_slot = true;

			put_in_matrix(golay_header1_extra_slot,0);

			size_t payload_len = 12 + 6 * data_reply_frame.data_queue_len;

			uint8_t nb_column = payload_len/3;

			unsigned char reply_frame[payload_len];
			memset(reply_frame,0,payload_len);

			matrix_linearization(reply_frame,nb_column);
			if(crc_state == 0) {
				uint32_t err = random_uint32_range(1,13);
				error = (float) err / 100.0;
				put_error_in_frame(reply_frame,payload_len,error,burst);
			}


			gnrc_waye_send_data_golay(netif, reply_frame, sink_addr, hello_frame.addr.u8, crc_state, payload_len);

			
			//gnrc_waye_send_data_golay(netif, (gnrc_waye_frame_data_golayed_t *)&data_reply_interleaver, sink_addr, hello_frame.addr.u8, crc_state);
			//gnrc_waye_send_data_golay(netif, data_reply_interleaver, sink_addr, hello_frame.addr.u8, crc_state,data_size);
			//gnrc_waye_send_data_golay(netif, (gnrc_waye_frame_data_golayed_t *)&data_golayed_frame, sink_addr, hello_frame.addr.u8,crc_state);
			rtt_set_alarm(wait_ack_extra_slot,waye_collect_slot_end_cb,netif);

		}
	
	}
	
}
static void _waye_hello_msg_handler(gnrc_netif_t *netif, msg_t *msg) {
	switch (msg->type) {
		case GNRC_WAYE_EVENT_RECV:
			waye_collect_send_handler(netif);
			break;

		case GNRC_WAYE_EVENT_SLOT_COLLECT:
			waye_collect_slot_end_handler(netif);
			break;

		case GNRC_WAYE_EVENT_SLOT_DISCOVERY:
			waye_neighbor_discovery_handler(netif);
			break;

		default:
			printf("[WAYE]: unknown message type 0x%04x"
					"(no message handler defined)\n", msg->type);
	}
}
static void * _waye_thread_data_collect(void *arg) {
	(void) arg;
	while(true){
		ztimer_sleep(ZTIMER_USEC,700);
		random_bytes((uint8_t*)&data[data_index].data,sizeof(data[data_index].data));
		data[data_index].timestamp = (uint16_t) rtt_get_counter();
		golay2412_encode(3, (unsigned char *)&data[data_index], data_golayed_frame.data_queue[data_index+2].data_golayed);

		data_len++;


		if(data_len <= GNRC_WAYE_DATA_REPLY_QUEUE_SIZE) {
			data_reply_frame.data_queue_len = data_len;
			golay_header2[2] = data_reply_frame.data_queue_len;

			golay2412_encode(3,golay_header2,data_golayed_frame.data_queue[1].data_golayed);
			put_in_matrix(data_golayed_frame.data_queue[1].data_golayed,1);
		}
		put_in_matrix( data_golayed_frame.data_queue[data_index+2].data_golayed,data_index+2);

		//data_index = (data_index + 1) % GNRC_WAYE_DATA_REPLY_QUEUE_SIZE;
		data_index = (data_len) % GNRC_WAYE_DATA_REPLY_QUEUE_SIZE;
	}
	return NULL;
}

static void put_in_matrix(unsigned char sample_golay_words[6], uint8_t position){
	uint8_t mask = 0x80;
	uint8_t index = 0;
	for(int i = 0 ; i < 2 ; i++){
		for(int j = 0 ; j < 24 ; j++){
			if(sample_golay_words[index] & mask){
				matrix[j][position*2+i] = 0x01;
			} else {
				matrix[j][position*2+i] = 0x00;
			}
			mask >>= 1;
			if(!mask){
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

static void put_error_in_frame(unsigned char * payload, uint8_t pay_len, float error_rate, uint8_t burst_size){
		if(error_rate > 1.0 ){ error_rate = 1.0 ; }
		uint16_t bit_nb = pay_len * 8;
		uint16_t bit_error_nb = floor(bit_nb * error_rate);
		unsigned char payload_mask[pay_len];
		uint8_t mask = 0x00, index = 0;
		uint32_t pos = 0;
		uint32_t desync_bit_time = 0;

		if (burst_size > bit_error_nb) { burst_size = random_uint32_range(1, bit_error_nb) ; }
		uint8_t original_burst_size = burst_size;

		memset(payload_mask, 0, pay_len);

		for(int i = 0 ; i < bit_error_nb ; ){
			pos = random_uint32_range(0, bit_nb-burst_size);
			index = pos / 8;
			mask = 0x80 >> pos % 8;
			bool already_changed = false ;
			if(i > bit_error_nb - burst_size) { burst_size = bit_error_nb - i ; }
			original_burst_size = burst_size;

			desync_bit_time = random_uint32_range(0,100);
			if(desync_bit_time > 30){
				burst_size = burst_size + 1;
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
		for(int i = 0 ; i < pay_len ; i++){
			payload[i] ^= payload_mask[i];
		}
}

void printu8(uint8_t d){
	uint8_t u, mask;
	mask = 1;
	mask <<= 7;
	for(u = 1; u<=8;u++){
		putchar(((d & mask)==0) ? '0' : '1' );
		d <<= 1;
	}
	printf(" ");
}


int main(void) {

	printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
	printf("This board features a(n) %s MCU.\n", RIOT_MCU);

	char line_buf[SHELL_DEFAULT_BUFSIZE];
	shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

	return 0;
}
