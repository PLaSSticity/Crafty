#!/bin/bash

#############
# Runs the test suit:
# HTM / PHTM / NV-HTM
#############
SAMPLES=1
NB_BENCH=1

# TODO: use -p 50
test[1]="./code/tpcc -t 60 -m 30 -w 30 -s 0 -d 2 -o 26 -p 70 -r 2 -n"
# test[1]="./code/tpcc -t 60 -m 10 -w 10 -s 0 -d 1 -o 99 -p 0 -r 0 -n"
# test[2]="./code/tpcc -t 60 -m 10 -w 10 -s 0 -d 20 -o 65 -p 15 -r 0 -n"

# drop labyrith, it does not had much info

test_name[1]="TPCC_H"
# test_name[1]="TPCC_H"
# test_name[2]="TPCC_L"

THREADS="1 2 4 8 12 13 14 16 20 24 26 27 28"

# 1GB
LOG_SIZE=40000000

BUILD_SCRIPT=./build-tpcc.sh
DATA_FOLDER=./dataTPCC
SCRIPTS_FOLDER=../../scripts
PLOT_FOLDER=./tests_nvm/plot

# dir where build.sh is
cd ..

mkdir -p $DATA_FOLDER

### Call with 1-program 2-threads
function bench {
	echo -ne "$2\n" >> parameters.txt
	ipcrm -M 0x00054321 >/dev/null
	timeout --signal=2 120s ${test[$1]} $2 >/dev/null
	# retry
	if [ $? -ne 0 ]; then
		pkill tpcc
		timeout --signal=2 120s ${test[$1]} $2  >/dev/null
	fi
	# retry
	if [ $? -ne 0 ]; then
		pkill tpcc
		timeout --signal=2 120s ${test[$1]} $2  >/dev/null
	fi
	pkill tpcc
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
				echo "$a - ${test[$i]} $t COMPLETE!\n"
			done
			mv parameters.txt     $DATA_FOLDER/"${test_name[$i]}"_"$1"_par_s"$a"
			mv stats_file         $DATA_FOLDER/"${test_name[$i]}"_"$1"_s"$a"
			mv stats_file.aux_thr $DATA_FOLDER/"${test_name[$i]}"_"$1"_aux_s"$a"
		done
		echo "${test_name[$i]} COMPLETE!\n"
	done
}

#MAKEFILE_ARGS="PERSISTENT_TM=0" $BUILD_SCRIPT stm-tinystm file # >/dev/null
#run_bench STM

MAKEFILE_ARGS="PERSISTENT_TM=1" $BUILD_SCRIPT stm-tinystm file # >/dev/null
run_bench PSTM

MAKEFILE_ARGS="SOLUTION=1 -j 14" \
$BUILD_SCRIPT htm-sgl-nvm file >/dev/null
run_bench HTM

# MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=4 LOG_SIZE=$LOG_SIZE -j 14" \
# $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
# run_bench NVHTM_W
#
# # NVHTM physical clocks, 2 threads sort,
# MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE \
# SORT_ALG=4 THRESHOLD=0.0 NDEBUG=1" \
# $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
# run_bench NVHTM_F

#MAKEFILE_ARGS="SOLUTION=3 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE \
#    SORT_ALG=4 THRESHOLD=0.0 NDEBUG=1" \
#    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
#run_bench NVHTM_F_LC

MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE \
SORT_ALG=5 FILTER=0.50 -j 14" \
$BUILD_SCRIPT htm-sgl-nvm file >/dev/null
run_bench NVHTM_B

MAKEFILE_ARGS="SOLUTION=2" \
$BUILD_SCRIPT htm-sgl-nvm file >/dev/null
run_bench PHTM

LOG_SIZE=500000 # wrap around 10 times
MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE \
SORT_ALG=5 FILTER=0.50 -j 14" \
$BUILD_SCRIPT htm-sgl-nvm file >/dev/null
run_bench NVHTM_B_10
#
# MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=2 LOG_SIZE=$LOG_SIZE \
# SORT_ALG=5 FILTER=0.50 THRESHOLD=0.5 -j 14" \
# $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
# run_bench NVHTM_R

# MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=1 LOG_SIZE=$LOG_SIZE \
# SORT_ALG=5 FILTER=0.50 -j 14" \
# $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
# run_bench NVHTM_P

# cd ./tests_nvm
# ./test_proc_STAMP.sh
#
# rm $DATA_FOLDER/*CAT $DATA_FOLDER/*CAT.cols
