#!/bin/bash

#############
# Compares the maximum capacity of:
# HTM / PHTM / NV-HTM
#
#############

SAMPLES=10

# BANK parameters
NB_TRANSFERS=1000000
BANK_BUDGET=100000
UPDATE_RATE=100
THREADS="1 2 4 6 8 12 16 20 24 28"
TX_SIZE="1"

LOG_SIZE=250000000

BUILD_SCRIPT=./build.sh
DATA_FOLDER=./dataNOCONFL
SCRIPTS_FOLDER=../../scripts
PLOT_FOLDER=./tests/plot
NAME="NOCONFL"

NB_ACCOUNTS=1024

# dir where build.sh is
cd ..

mkdir -p $DATA_FOLDER

### Call with 1-TX_SIZE 2-UPDATE_RATE 3-THREADS 4-NB_ACCOUNTS 5-NB_TRANSFERS
function run_bench {
	echo -ne "1\t1\t$4\t$2\t$1\n" >> parameters.txt
	ipcrm -M 0x00054321 >/dev/null
	timeout 10m ./bank/bank -a $4 -d $5 -b $BANK_BUDGET -u $2 -n $3 \
        -s $1 -q -c >/dev/null
}

function bench {
	for i in `seq $SAMPLES`
	do
		rm -f parameters.txt stats_file stats_file.aux_thr
        echo -ne "#SEQ\tNO_CONFL\tACCOUNTS\tUPDATE_RATE\tSIZE_TX\n" > parameters.txt
		for j in $THREADS
		do
            run_bench $TX_SIZE $UPDATE_RATE $j $NB_ACCOUNTS $NB_TRANSFERS
		done
        mv parameters.txt     $DATA_FOLDER/"$1"_par_s"$i"
        mv stats_file         $DATA_FOLDER/"$1"_s"$i"
        mv stats_file.aux_thr $DATA_FOLDER/"$1"_aux_s"$i"
	done
}

# MAKEFILE_ARGS="SOLUTION=1 CACHE_ALIGN_POOL=1 NDEBUG=1" \
#    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
# bench HTM

# NVHTM physical clocks, 2 threads sort,
# MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE \
#    SORT_ALG=4 THRESHOLD=0.0 CACHE_ALIGN_POOL=1" \
#    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
# bench NVHTM

# NVHTM logical clocks, 2 threads sort,
# MAKEFILE_ARGS="SOLUTION=3 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE \
#     SORT_ALG=4 THRESHOLD=0.0 CACHE_ALIGN_POOL=1" \
#     $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
# bench NVHTM_LC

# Same but WRAP
MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=4 LOG_SIZE=$LOG_SIZE \
    SORT_ALG=4 THRESHOLD=0.0 CACHE_ALIGN_POOL=1 NDEBUG=1" \
    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
bench NVHTM_W

MAKEFILE_ARGS="SOLUTION=3 DO_CHECKPOINT=4 LOG_SIZE=$LOG_SIZE \
    SORT_ALG=4 THRESHOLD=0.0 CACHE_ALIGN_POOL=1 NDEBUG=1" \
    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
bench NVHTM_LC_W

# Same but BACKWARD
# MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE \
#    SORT_ALG=5 FILTER=0.99 CACHE_ALIGN_POOL=1" \
#    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
# bench NVHTM_B

# MAKEFILE_ARGS="SOLUTION=3 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE \
#    SORT_ALG=5 FILTER=0.99 CACHE_ALIGN_POOL=1" \
#    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
# bench NVHTM_LC_B

#MAKEFILE_ARGS="SOLUTION=2 CACHE_ALIGN_POOL=1 NDEBUG=1" \
#    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
#bench PHTM

cd ./tests
./test_proc_NO_CONFL.sh
