# Set the name of your application:
APPLICATION = test_counter

# If no BOARD is found in the environment, use this default:
BOARD ?= openmote-b

# This has to be the absolute path to the RIOT base directory:
RIOTBASE ?= $(CURDIR)/../../..


CFLAGS += -DRFCTL_ANTENNA_DEFAULT=RFCTL_ANTENNA_BOARD
#CFLAGS += -DRFCTL_ANTENNA_DEFAULT=RFCTL_ANTENNA_EXT

#CFLAGS += -DIEEE802154_DEFAULT_TXPOWER=-17
CFLAGS += -DIEEE802154_DEFAULT_CHANNEL=25

# Uncomment this to enable scheduler statistics for ps:
#CFLAGS += -DSCHEDSTATISTICS

# If you want to use native with valgrind, you should recompile native
# with the target all-valgrind instead of all:
# make -B clean all-valgrind

# Uncomment this to enable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP = 1

# Change this to 0 to show compiler invocation lines by default:
QUIET ?= 1

# Modules to include:

#USEMODULE += ieee802154

USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += xtimer
USEMODULE += gnrc
USEMODULE += gnrc_netdev_default
USEMODULE += auto_init_gnrc_netif

# If your application is very simple and doesn't use modules that use
# messaging, it can be disabled to save some memory:

#DISABLE_MODULE += core_msg

#export INCLUDES += -Iapplication_include

# Specify custom dependencies for your application here ...
# APPDEPS = app_data.h config.h

include $(RIOTBASE)/Makefile.include

# ... and define them here (after including Makefile.include,
# otherwise you modify the standard target):
#proj_data.h: script.py data.tar.gz
#	./script.py
