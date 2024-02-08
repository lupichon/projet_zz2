#include <stdio.h>
#include "thread.h"
#include "shell.h"

int main(void)
{
	//enable network interface 
	//gnrc_netif_init_devs();
	puts("we are inside main");
	printf("You are running RIOT on a(n) %s board.\n", RIOT_BOARD);
	printf("This board features a(n) %s MCU.\n", RIOT_MCU);

	char line_buf[SHELL_DEFAULT_BUFSIZE];
	shell_run(NULL, line_buf, SHELL_DEFAULT_BUFSIZE);
	return 0;
}