#ifndef RAW_SEND_H
#define RAW_SEND_H

/*
 * Function for sending raw IEEE 802.15.4 frames
 *
 * Parameters :
 *   data :      pointer to payload
 *   len :       payload size
 *   addr_str :  Destination address string
 *   iface_pid : PID of the network interface's thread
 *               KERNEL_PID_UNDEF for the first interface
 *
 * Returns 0 if success, 1 otherwise
 */
int send_frame(const void *data, size_t len, const char *addr_str, kernel_pid_t iface_pid);

#endif /* RAW_SEND_H */
