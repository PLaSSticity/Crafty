# TODO: optional flags defines
OPT ?= -O0
USE_P8 ?= 0
USE_MIN_NVM ?= 1
LOG_SIZE ?= 100

####
DO_CHECKPOINT ?= 3  
####
# Available checkpoints:
#  1 - periodic
#  2 - reactive
#  3 - no_manager
####

####
SOLUTION ?= 3
####
# Available solutions:
#  1 - HTM_ONLY
#  2 - AVNI
#  3 - REDO_COUNTER
#  4 - REDO_TS
####

ifeq ($(SOLUTION),1)
include Makefile_htm_only.inc
DEFINES  += -DHTM_ONLY
endif

ifeq ($(SOLUTION),2)
include Makefile_avni.inc
DEFINES  += -DAVNI_SOL
endif

FLAG_CHECKPOINT :=

ifneq ($(DO_CHECKPOINT),0)
FLAG_CHECKPOINT += -DDO_CHECKPOINT=$(DO_CHECKPOINT)
endif

ifeq ($(SOLUTION),3)
include Makefile_redo_counter.inc
DEFINES  += -DREDO_COUNTER -DVALIDATION=2 $(FLAG_CHECKPOINT)
endif

ifeq ($(SOLUTION),4)
include Makefile_redo_ts.inc
DEFINES  += -DREDO_TS -DVALIDATION=3 $(FLAG_CHECKPOINT)
endif

ifeq ($(USE_P8),1)
DEFINES  += -DUSE_P8 -mhtm
else
DEFINES  += -mrtm
endif

ifeq ($(USE_MIN_NVM),1)
DEFINES  += -DUSE_MIN_NVM
endif

GCC_MAJOR:=$(shell gcc -dumpversion | cut -d'.' -f1)

ifeq ($(GCC_MAJOR),4)
DEFINES += -DOLD_ALLOC
endif

DEFINES += -DNVMHTM_LOG_SIZE=$(LOG_SIZE)
