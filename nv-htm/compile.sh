#!/bin/bash

if [[ $# -lt 1 ]] ; then
	echo "Usage: $0 <solution> [rec_type] [log_size] [...]"
	echo "  solution:       HTM|AVNI|REDO_COUNTER|REDO_TS|CRAFTY"
	echo "                  (default is REDO_COUNTER)"
	echo "  rec_type:       REACTIVE|PERIODIC"
	echo "  log_size:       Number of entries in the per thread logs"
	exit
fi

SOLUTION=3
DO_CHECKPOINT=0
LOG_SIZE=1000

if [[ $1 == "HTM" ]] ; then
	SOLUTION=1
elif [[ $1 == "AVNI" ]] ; then
	SOLUTION=2
elif [[ $1 == "REDO_COUNTER" ]] ; then
	SOLUTION=3
elif [[ $1 == "REDO_TS" ]] ; then
	SOLUTION=4
elif [[ $1 == "CRAFTY" ]] ; then
	SOLUTION=5
fi

# WARNING CHECKPOINT DISABLED

if [[ $2 == "PERIODIC" ]] ; then
	DO_CHECKPOINT=1
elif [[ $2 == "REACTIVE" ]] ; then
	DO_CHECKPOINT=2
elif [[ $2 == "NO_MANAGER" ]] ; then
	DO_CHECKPOINT=3
elif [[ $2 == "WRAP" ]] ; then
	DO_CHECKPOINT=4
elif [[ $2 == "FORK" ]] ; then
	DO_CHECKPOINT=5
fi

if [[ $# -gt 2 ]] ; then
	LOG_SIZE=$3
fi

LIB_PMEM_PATH="~/libs/pmdk"
LIB_MIN_NVM_PATH="~/projs/minimal_nvm"

make clean && make -j56 LIB_PMEM_PATH=$LIB_PMEM_PATH \
	LIB_MIN_NVM_PATH=$LIB_MIN_NVM_PATH USE_P8=0 \
	SOLUTION=$SOLUTION OPT="-O0" LOG_SIZE=$LOG_SIZE \
	DO_CHECKPOINT=$DO_CHECKPOINT \
