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
#include "thread.h"
#include "xtimer.h"
#include "net/gnrc.h"
#include "net/gnrc/netreg.h"
#include "shell.h"
#include "led.h"

#include "raw_send.h"

#define TOS_AM_GROUP (6)
#define Q_SZ (8)

typedef struct __attribute__((packed)) {
	uint8_t amgroup;
	uint8_t padding;
	uint8_t counter;
} ctr_msg_t;

char recv_thread_stack[THREAD_STACKSIZE_MAIN];
char send_thread_stack[THREAD_STACKSIZE_MAIN];

void *_handle_incoming_pkt(gnrc_pktsnip_t *pkt) {
	(void) pkt;
	ctr_msg_t *ctr_msg;

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
	gnrc_netreg_entry_t me_reg = { .demux_ctx = GNRC_NETREG_DEMUX_CTX_ALL, .target.pid = thread_getpid() };
	gnrc_netreg_register(GNRC_NETTYPE_UNDEF, &me_reg);

	while (1) {
		msg_receive(&msg);
		++ctr;
		printf("Message received %d\n", ctr);
		switch (msg.type) {
			case GNRC_NETAPI_MSG_TYPE_RCV:
				xtimer_usleep(2000);
				pkt = msg.content.ptr;
				send_frame(pkt, sizeof(ctr_msg_t), "10:6e", 0);
				//send_frame(&message2, sizeof(ctr_msg_t), pkt, 0);
				printf("Sending Back the reception counter but just to 10:6e for now ... : %u of size : %d\n ", message2.counter,sizeof(ctr_msg_t));
				++message2.counter;
				//_handle_incoming_pkt(pkt);
				break;
			case GNRC_NETAPI_MSG_TYPE_SND:
				pkt = msg.content.ptr;
				//_handle_outgoing_pkt(pkt);
				break;
			case GNRC_NETAPI_MSG_TYPE_SET:
			case GNRC_NETAPI_MSG_TYPE_GET:
				msg_reply(&msg, &reply);
				break;
			default:
				break;
		}
	}
	return NULL;
}

void *_send_thread(void *arg) {
	(void) arg;
	ctr_msg_t message;

	message.amgroup = TOS_AM_GROUP;
	message.counter = 0;

	while (1) {
		send_frame(&message, sizeof(ctr_msg_t), "ff:ff", 0);
		// printf("Counter : %u\n", message.counter);
		++message.counter;
		xtimer_sleep(1);
	}
	return NULL;
}

int main(void) {
	printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
	printf("This board features a(n) %s MCU.\n", RIOT_MCU);

	thread_create(recv_thread_stack, sizeof(recv_thread_stack), THREAD_PRIORITY_MAIN - 2, THREAD_CREATE_STACKTEST, _recv_thread, NULL, "recv thread");

	//thread_create(send_thread_stack, sizeof(send_thread_stack), THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST, _send_thread, NULL, "counter thread");

	char line_buf[SHELL_DEFAULT_BUFSIZE];
	shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);

	return 0;
}
