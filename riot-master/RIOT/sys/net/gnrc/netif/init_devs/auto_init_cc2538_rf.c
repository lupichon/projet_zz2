/*
 * Copyright (C) 2016 MUTEX NZ Ltd
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 *
 */

/*
 * @ingroup sys_auto_init_gnrc_netif
 * @{
 *
 * @file
 * @brief   Auto initialization for the cc2538 network interface
 *
 * @author  Aaron Sowry <aaron@mutex.nz>
 */

#include "log.h"
#include "net/gnrc/netif/ieee802154.h"
#include "include/init_devs.h"

#include "cc2538_rf.h"
#define GNRCHELLOS (1)
//#ifdef MODULE_GNRC_HELLOS
#ifdef GNRCHELLOS
#include "net/gnrc/waye/waye.h"
#endif
#include "net/netdev/ieee802154_submac.h"
#define ENABLE_DEBUG 1
#include "debug.h"

/**
 * @brief   Define stack parameters for the MAC layer thread
 * @{
 */
#define CC2538_MAC_STACKSIZE       (IEEE802154_STACKSIZE_DEFAULT)
#ifndef CC2538_MAC_PRIO
#define CC2538_MAC_PRIO            (GNRC_NETIF_PRIO)
#endif

static netdev_ieee802154_submac_t cc2538_rf_netdev;
static char _cc2538_rf_stack[CC2538_MAC_STACKSIZE];
static gnrc_netif_t _netif;

void auto_init_cc2538_rf(void)
{
    LOG_DEBUG("[auto_init_netif] initializing cc2538 radio\n");
    DEBUG("Do we pass here?\n");
    netdev_register(&cc2538_rf_netdev.dev.netdev, NETDEV_CC2538, 0);
    netdev_ieee802154_submac_init(&cc2538_rf_netdev);
    cc2538_rf_hal_setup(&cc2538_rf_netdev.submac.dev);

    cc2538_init();
#if defined GNRCHELLOS
	printf("JE SUIS BIEN RENTRE\n");
	        gnrc_netif_waye_create(&_netif, _cc2538_rf_stack,
                                 CC2538_MAC_STACKSIZE,
                                 CC2538_MAC_PRIO, "cc2538_waye",
                                 &cc2538_rf_netdev.dev.netdev);
#else
    gnrc_netif_ieee802154_create(&_netif, _cc2538_rf_stack,
                                 CC2538_MAC_STACKSIZE,
                                 CC2538_MAC_PRIO, "cc2538_rf",
                                 &cc2538_rf_netdev.dev.netdev);
#endif
}
/** @} */
