# Set the name of your application:
APPLICATION = waye

# If no BOARD is found in the environment, use this default:
BOARD ?= samr21-xpro

# This has to be the absolute path to the RIOT base directory:

RIOTBASE ?= $(CURDIR)/../../..


CHANNEL ?= 25		# Default : 26
PANID ?= 0x0023		# Default : 0x0023
TXPOWER ?= 0		# Default : 0 dBm
#TXPOWER ?= -17

#ANTENNA ?= BOARD
ANTENNA ?= EXT


CFLAGS += -DIEEE802154_DEFAULT_CHANNEL=$(CHANNEL)
CFLAGS += -DIEEE802154_DEFAULT_PANID=$(PANID)
CFLAGS += -DIEEE802154_DEFAULT_TXPOWER=$(TXPOWER)
CFLAGS += -DRFCTL_ANTENNA_DEFAULT=RFCTL_ANTENNA_$(ANTENNA)

# Increase ISR stack size
#CFLAGS += -DISR_STACKSIZE=1024U

# Uncomment this to enable scheduler statistics for ps:
#CFLAGS += -DSCHEDSTATISTICS

# If you want to use native with valgrind, you should recompile native
# with the target all-valgrind instead of all:
# make -B clean all-valgrind

# Uncomment this to enable code in RIOT that does safety checking
# which is not needed in a production environment but helps in the
# development process:
DEVELHELP = 1

WERROR = 0

# Change this to 0 to show compiler invocation lines by default:
QUIET ?= 1

# Modules to include:

USEMODULE += netdev_default
USEMODULE += gnrc_waye
USEMODULE += auto_init_gnrc_netif
#USEMODULE += gnrc_netif_init_devs

USEMODULE += sba_sp6t

USEMODULE += ecc_golay2412
USEMODULE += ecc_interleaver
USEMODULE += ecc_hamming256
USEMODULE += ecc_repetition
USEMODULE += periph_rtt
USEMODULE += ztimer_sec
USEMODULE += ztimer_msec
USEMODULE += ztimer_usec

# Modules useful during development
USEMODULE += shell
USEMODULE += shell_commands
USEMODULE += ps
USEMODULE += od


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
