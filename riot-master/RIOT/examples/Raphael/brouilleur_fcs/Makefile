
# name of your application
APPLICATION = brouilleur_fcs

# If no BOARD is found in the environment, use this default:
BOARD ?= openmote-b

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../..

#CFLAGS += -DRFCTL_ANTENNA_DEFAULT=RFCTL_ANTENNA_BOARD
CFLAGS += -DRFCTL_ANTENNA_DEFAULT=RFCTL_ANTENNA_EXT

#CFLAGS += -DIEEE802154_DEFAULT_TXPOWER=-17
CFLAGS += -DIEEE802154_DEFAULT_CHANNEL=25

# Include packages that pull up and auto-init the link layer.
USEMODULE += netdev_default
USEMODULE += auto_init_gnrc_netif
USEMODULE += gnrc_mac
USEMODULE += ztimer_usec

# Add also the shell, some shell commands
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps


# Comment this out to disable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP ?= 1

# Change this to 0 show compiler invocation lines by default:
QUIET ?= 1

include $(RIOTBASE)/Makefile.include

# Set a custom channel if needed
include $(RIOTMAKE)/default-radio-settings.inc.mk
