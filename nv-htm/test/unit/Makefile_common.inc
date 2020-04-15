LIB_PMEM_PATH ?= ~/libs/nvml
LIB_MIN_NVM_PATH ?= ~/projs/minimal_nvm

COMMON_SRC:=$(shell ls -rt -d -1 ../../common/src/*.c ../../common/src/*.cpp)
COMMON_INC:=-I ../../common/ \
	-I $(LIB_PMEM_PATH)/src/include \
#	-I $(LIB_MIN_NVM_PATH)/include \
#
COMMON_LIBS:=-L $(LIB_PMEM_PATH)/src/debug -l pmem \
#	-L $(LIB_MIN_NVM_PATH)/bin -l minimal_nvm \
#
