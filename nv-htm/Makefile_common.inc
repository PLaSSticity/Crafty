# LIB_PMEM_PATH ?= ~/libs/pmdk
LIB_MIN_NVM_PATH ?= ../nvm-emulation
ARCH_DEP_PATH ?= ../arch-dep
HTM_ALG_DEP_PATH ?= ../htm-alg

CPU_MAX_FREQ=$(shell cat /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq || lscpu | sed -n 's/CPU MHz: *\([0-9]*\)[.][0-9]*/\1000/p')
MAX_PHYS_THRS=$(shell cat /proc/cpuinfo | grep processor | wc -l)

DEFINES += -DCPU_MAX_FREQ=$(CPU_MAX_FREQ) \
-DMAX_PHYS_THRS=$(MAX_PHYS_THRS)

# -I $(LIB_PMEM_PATH)/src/include \
# 

COMMON_SRC:=$(shell ls -rt -d -1 $(ROOT)/common/src/*.c $(ROOT)/common/src/*.cpp)
COMMON_INC:=-I $(ROOT)/common/ \
-I $(LIB_MIN_NVM_PATH)/include \
-I $(ARCH_DEP_PATH)/include \
-I $(HTM_ALG_DEP_PATH)/include \
#
COMMON_LIBS:=-L $(LIB_PMEM_PATH)/src/debug -l pmem \
-L $(LIB_MIN_NVM_PATH)/bin -l minimal_nvm \
-L $(HTM_ALG_DEP_PATH)/bin -l htm_sgl \
# -ltcmalloc \
#
