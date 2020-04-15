#!/bin/bash

#############
# Runs the test suit:
# HTM / PHTM / NV-HTM
#############

SAMPLES=1
NB_BENCH=1

test[1]="./ssca2/ssca2 -s14 -i1.0 -u1.0 -l9 -p9 -t"
test[2]="./kmeans/kmeans -m40 -n40 -t0.05 -i ./kmeans/inputs/random-n65536-d32-c16.txt -p" # low
test[3]="./genome/genome -g512 -s32 -n32768 -t"
test[4]="./vacation/vacation -n2 -q90 -u98 -r1048576 -t4194304 -a 1 -c"  # low
test[5]="./intruder/intruder -a10 -l16 -n4096 -s1 -t"
test[6]="./yada/yada -a10 -i ./yada/inputs/ttimeu10000.2 -t"
test[7]="./vacation/vacation -n4 -q60 -u90 -r1048576 -t4194304 -a 1 -c"  # high
test[8]="./kmeans/kmeans -m15 -n15 -t0.05 -i ./kmeans/inputs/random-n65536-d32-c16.txt -p" # high
test[9]="./labyrinth/labyrinth -i labyrinth/inputs/random-x48-y48-z3-n64.txt -t"
test[10]="./bayes/bayes -v32 -r4096 -n2 -p20 -i2 -e2 -t"

# drop labyrith, it does not had much info

test_name[1]="SSCA2"
test_name[2]="KMEANS_LOW"
test_name[3]="GENOME"
test_name[4]="VACATION_LOW"
test_name[5]="INTRUDER"
test_name[6]="YADA"
test_name[7]="VACATION_HIGH"
test_name[8]="KMEANS_HIGH"
test_name[9]="LABYRINTH"
test_name[10]="BAYES"

#THREADS="1 2 4 8 12 13 14 16 22 24 26 27 28"
THREADS="1 2 4"

# 1GB
LOG_SIZE=500000000

BUILD_SCRIPT=./build-stamp.sh
DATA_FOLDER=./dataSTAMP
SCRIPTS_FOLDER=../../scripts
PLOT_FOLDER=./tests_nvm/plot

# dir where build.sh is
cd ..

mkdir -p $DATA_FOLDER

### Call with 1-program 2-threads
function bench {
	echo -ne "$2\n" >> parameters.txt
	ipcrm -M 0x00054321 >/dev/null
	timeout --signal=2 5s ${test[$1]} $2 >/dev/null
    # retry
    if [ $? -ne 0 ]; then
        timeout --signal=2 5s ${test[$1]} $2  >/dev/null
    fi
    # retry
    if [ $? -ne 0 ]; then
        timeout --signal=2 10s ${test[$1]} $2  >/dev/null
    fi
    # retry
    if [ $? -ne 0 ]; then
        timeout --signal=2 30s ${test[$1]} $2  >/dev/null
    fi
    # retry
    if [ $? -ne 0 ]; then
        timeout --signal=2 1m ${test[$1]} $2  >/dev/null
    fi
    # retry
    if [ $? -ne 0 ]; then
        timeout --signal=2 15m ${test[$1]} $2  >/dev/null
    fi
}

function run_bench {
    rm -f parameters.txt stats_file stats_file.aux_thr
    for i in `seq $NB_BENCH`
    do
        for a in `seq $SAMPLES`
        do
            echo -ne "#THREADS\n" > parameters.txt
			for t in $THREADS
			do
				# rm -f /tmp/*.socket
				bench $i $t
                wait ;
                sleep 0.01
                echo "$a - ${test[$i]} $t COMPLETE!\n"
			done
            mv parameters.txt     $DATA_FOLDER/"${test_name[$i]}"_"$1"_par_s"$a"
            mv stats_file         $DATA_FOLDER/"${test_name[$i]}"_"$1"_s"$a"
            mv stats_file.aux_thr $DATA_FOLDER/"${test_name[$i]}"_"$1"_aux_s"$a"
		done
		echo "${test_name[$i]} COMPLETE!\n"
	done
}

#MAKEFILE_ARGS="" \
#    $BUILD_SCRIPT stm-tinystm file >/dev/null
#run_bench STM

# MAKEFILE_ARGS="PERSISTENT_TM=1" \
#    $BUILD_SCRIPT stm-tinystm file
#run_bench PSTM

#MAKEFILE_ARGS="SOLUTION=1 NDEBUG=1" \
#    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
#run_bench HTM

MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=4 LOG_SIZE=$LOG_SIZE \
    NDEBUG=1" \
    $BUILD_SCRIPT htm-sgl-nvm file
run_bench NVHTM_W

# NVHTM physical clocks, 2 threads sort,
MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE \
    SORT_ALG=4 THRESHOLD=0.0 NDEBUG=1" \
    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
run_bench NVHTM_F

#MAKEFILE_ARGS="SOLUTION=3 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE \
#    SORT_ALG=4 THRESHOLD=0.0 NDEBUG=1" \
#    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
#run_bench NVHTM_F_LC

MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE \
    SORT_ALG=5 FILTER=0.50 FILTER=0.1 NDEBUG=1" \
    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
run_bench NVHTM_B

#MAKEFILE_ARGS="SOLUTION=2 NDEBUG=1" \
#    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
#run_bench PHTM

cd ./tests_nvm
./test_proc_STAMP.sh

rm $DATA_FOLDER/*CAT $DATA_FOLDER/*CAT.cols
