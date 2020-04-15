# TODO: optional flags defines
OPT ?= -O0
DEFINES += -flto
USE_P8 ?= 0
USE_MIN_NVM ?= 1
LOG_SIZE ?= 10000
# percentage of the log to free-up
THRESHOLD ?= 0.0
# sleep time of the log manager (nano-seconds)
PERIOD ?= 10
ROOT ?= ./

FILTER ?= 0.50
BUDGET ?= 20
IS_BATCH ?= 0

ifeq ($(NVM_LATENCY_RDTSCP),1)
    DEFINES += -DNVM_LATENCY_RDTSCP=1 -DNVM_WAIT_CYCLES=$(NVM_WAIT_CYCLES)
endif

DEFINES += -DNVMHTM_LOG_SIZE=$(LOG_SIZE) \
    -DSORT_ALG=$(SORT_ALG) \
    -DLOG_FILTER_THRESHOLD=$(FILTER) \
    -DHTM_SGL_INIT_BUDGET=$(BUDGET) \
    -DNVM_LATENCY_NS=$(NVM_LATENCY)

####
DO_CHECKPOINT ?= 0
####
# Available checkpoints:
#  1 - periodic
#  2 - reactive
#  3 - no_manager
#  4 - wrap
#  5 - fork
####

####
SOLUTION ?= 3
####
# Available solutions:
#  1 - HTM
#  2 - PHTM
#  3 - NVHTM Logical Clock
#  4 - NVHTM Physical Clock
#  5 - Crafty
####

####
SORT_ALG ?= 4
####
# Available solutions:
#  1 - Using a map/set to sort the logs
#  2 - Array
#  3 - No sort - apply next
#  4 - 2 threads: sort + apply
#  5 - Backward approach
####

ifeq ($(NDEBUG),1)
DEFINES += -DNDEBUG=1
endif

ifeq ($(CRAFTY_STATS),1)
DEFINES += -DCRAFTY_STATS=1
endif

ifeq ($(VERBOSE),1)
DEFINES += -DVERBOSE=1
endif

ifeq ($(CRAFTY_REDO),1)
DEFINES += -DCRAFTY_REDO=1
endif

ifeq ($(CRAFTY_VALIDATE),1)
DEFINES += -DCRAFTY_VALIDATE=1
endif

ifeq ($(RETRY_NO_DELAY),1)
DEFINES += -DRETRY_NO_DELAY=1
endif

ifeq ($(ROLLOVER_BITS),1)
DEFINES += -DROLLOVER_BITS=1
endif

USE_MALLOC ?= 0

ifeq ($(USE_MALLOC),1)
DEFINES  += -DUSE_MALLOC
endif

ifeq ($(SOLUTION),1)
include $(ROOT)/Makefile_htm_only.inc
DEFINES  += -DHTM_ONLY
endif

ifeq ($(SOLUTION),2)
include $(ROOT)/Makefile_phtm.inc
DEFINES  += -DAVNI_SOL
endif

FLAG_CHECKPOINT :=

ifneq ($(DO_CHECKPOINT),0)
FLAG_CHECKPOINT += -DDO_CHECKPOINT=$(DO_CHECKPOINT)
FLAG_CHECKPOINT += -DLOG_THRESHOLD=$(THRESHOLD)
FLAG_CHECKPOINT += -DLOG_PERIOD=$(PERIOD)
endif

ifeq ($(SOLUTION),3)
include $(ROOT)/Makefile_nvhtm_lc.inc
DEFINES  += -DREDO_COUNTER -DVALIDATION=2 $(FLAG_CHECKPOINT)
endif

ifeq ($(SOLUTION),4)
include $(ROOT)/Makefile_nvhtm_pc.inc
DEFINES  += -DREDO_TS -DVALIDATION=3 $(FLAG_CHECKPOINT)
endif

ifeq ($(SOLUTION),5)
include $(ROOT)/Makefile_crafty.inc
DEFINES    += -DCRAFTY
endif

ifeq ($(USE_P8),1)
# __powerpc__ already defined. Drop USE_P8
DEFINES  += -DUSE_P8 -mhtm
else
DEFINES  += -mrtm
endif

ifeq ($(USE_MIN_NVM),1)
# the other one is bugged
DEFINES  += -DUSE_MIN_NVM
endif

GCC_MAJOR:=$(shell gcc -dumpversion | cut -d'.' -f1)

ifeq ($(GCC_MAJOR),4)
DEFINES += -DOLD_ALLOC
endif

ifeq ($(IS_BATCH),1)
DEFINES += -DAPPLY_BATCH_TX
endif
