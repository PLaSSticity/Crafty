#!/bin/bash

#############
# Compares the backward performance
#############

SAMPLES=3

# BANK parameters
NB_TRANSFERS=50000000 #50000000
BANK_BUDGET=1000000
UPDATE_RATE="10 50 90"
THREADS=7
TX_SIZE=2
ITEMS_READ="10 400"

LOG_SIZE=100000

BUILD_SCRIPT=./build.sh
DATA_FOLDER=./dataBACKWARD
SCRIPTS_FOLDER=../../scripts
PLOT_FOLDER=./tests/plot

NB_ACCOUNTS=2048

# dir where build.sh is
cd ..

mkdir -p $DATA_FOLDER

### Call with 1-TX_SIZE 2-UPDATE_RATE 3-THREADS 4-NB_ACCOUNTS 5-NB_TRANSFERS 6-ITEMS_READ
function run_bench {
	echo -ne "0\t0\t$4\t$2\t$1\n" >> parameters.txt
	ipcrm -M 0x00054321 >/dev/null
	timeout 10m ./bank/bank -a $4 -d $5 -b $BANK_BUDGET -u $2 -n $3 -s $1 -r $6 -q >/dev/null
    # retry twice
    if [ $? -ne 0 ]; then
        timeout 15m ./bank/bank -a $4 -d $5 -b $BANK_BUDGET -u $2 -n $3 -s $1 -r $6 -q >/dev/null
    fi
}

function bench {
	for i in `seq $SAMPLES` 
	do
		rm -f parameters.txt stats_file stats_file.aux_thr
        echo -ne "#SEQ\tNO_CONFL\tACCOUNTS\tUPDATE_RATE\tSIZE_TX\n" > parameters.txt
		for j in $UPDATE_RATE
		do
            run_bench $TX_SIZE $j $THREADS $NB_ACCOUNTS $NB_TRANSFERS "$2"
		done
        mv parameters.txt     $DATA_FOLDER/"$1"_par_s"$i"
        mv stats_file         $DATA_FOLDER/"$1"_s"$i"
        mv stats_file.aux_thr $DATA_FOLDER/"$1"_aux_s"$i"
	done
}

for r in $ITEMS_READ 
do

#MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE \
#    SORT_ALG=5 FILTER=0.01 CACHE_ALIGN_POOL=1 NDEBUG=1" \
#    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
#bench NVHTM0_01

#MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE \
#    SORT_ALG=5 FILTER=0.05 CACHE_ALIGN_POOL=1 NDEBUG=1" \
#    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
#bench NVHTM0_05

#MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE \
#    SORT_ALG=5 FILTER=0.1 CACHE_ALIGN_POOL=1 NDEBUG=1" \
#    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
#bench NVHTM0_1

MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE \
    SORT_ALG=5 FILTER=0.5 CACHE_ALIGN_POOL=1 NDEBUG=1" \
    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
bench NVHTM0_5_"$r" $r

#MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE \
#    SORT_ALG=5 FILTER=0.9 CACHE_ALIGN_POOL=1 NDEBUG=1" \
#    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
#bench NVHTM0_9

MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE \
    SORT_ALG=4 THRESHOLD=0.0 CACHE_ALIGN_POOL=1 NDEBUG=1" \
    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
bench NVHTM_F_"$r" $r

MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=4 LOG_SIZE=$LOG_SIZE \
    CACHE_ALIGN_POOL=1 NDEBUG=1" \
    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
bench NVHTM_W_"$r" $r

MAKEFILE_ARGS="SOLUTION=2 \
    CACHE_ALIGN_POOL=1  NDEBUG=1" \
    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
bench PHTM_"$r" $r

######################
MAKEFILE_ARGS="SOLUTION=1 CACHE_ALIGN_POOL=1 NDEBUG=1" \
    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
bench HTM_"$r" $r
######################

done

cd ./tests/
./test_proc_backward.sh
cd ..

mv dataBACKWARD data/dataBACKWARD

THREADS=28
for r in $ITEMS_READ 
do
    MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE \
        SORT_ALG=5 FILTER=0.5 CACHE_ALIGN_POOL=1 NDEBUG=1" \
        $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
    bench NVHTM0_5_"$r" $r

    MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE \
        SORT_ALG=4 THRESHOLD=0.0 CACHE_ALIGN_POOL=1 NDEBUG=1" \
        $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
    bench NVHTM_F_"$r" $r

    MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=4 LOG_SIZE=$LOG_SIZE \
        CACHE_ALIGN_POOL=1 NDEBUG=1" \
        $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
    bench NVHTM_W_"$r" $r

    MAKEFILE_ARGS="SOLUTION=1 CACHE_ALIGN_POOL=1 NDEBUG=1" \
        $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
    bench HTM_"$r" $r
done

cd ./tests/
./test_proc_backward.sh
cd ..
