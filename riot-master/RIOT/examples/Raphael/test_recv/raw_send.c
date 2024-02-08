#include <stdio.h>
#include <string.h>
#include "net/gnrc.h"
#include "net/gnrc/netif.h"
#include "net/gnrc/netif/hdr.h"

#include "raw_send.h"

int send_frame(const void *data, size_t len, const char *addr_str, kernel_pid_t iface_pid) {
	gnrc_netif_t *iface;
	uint8_t addr[GNRC_NETIF_L2ADDR_MAXLEN];
	size_t addr_len;
	gnrc_pktsnip_t *pkt, *hdr;
	gnrc_netif_hdr_t *nethdr;
	uint8_t flags = 0x00;

	/* use first interface by default */
	if (iface_pid == KERNEL_PID_UNDEF) {
		iface = gnrc_netif_iter(NULL);

		if (iface == NULL) {
			printf("error: no interface available");
			return 1;
		} else {
			iface_pid = iface->pid;
		}
	}
	/* check if given interface PID is valid */
	else {
		if (gnrc_netif_get_by_pid(iface_pid) == NULL) {
			puts("error: invalid interface given");
			return 1;
		}
	}

	/* parse address */
	addr_len = gnrc_netif_addr_from_str(addr_str, addr);

	if (addr_len == 0) {
		puts("error: invalid address given");
		return 1;
	}

	/* put packet together */
	pkt = gnrc_pktbuf_add(NULL, data, len, GNRC_NETTYPE_UNDEF);
	if (pkt == NULL) {
		puts("error: packet buffer full");
		return 1;
	}
	hdr = gnrc_netif_hdr_build(NULL, 0, addr, addr_len);
	if (hdr == NULL) {
		puts("error: packet buffer full");
		gnrc_pktbuf_release(pkt);
		return 1;
	}
	LL_PREPEND(pkt, hdr);
	nethdr = (gnrc_netif_hdr_t *)hdr->data;
	nethdr->flags = flags;
	/* and send it */
	if (gnrc_netapi_send(iface_pid, pkt) < 1) {
		puts("error: unable to send");
		gnrc_pktbuf_release(pkt);
		return 1;
	}

	return 0;
}
