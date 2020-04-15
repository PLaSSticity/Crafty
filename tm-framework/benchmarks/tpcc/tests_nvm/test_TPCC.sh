#!/bin/bash

#############
# Runs the test suit:
# HTM / PHTM / NV-HTM
#############

SAMPLES=4
NB_BENCH=2

# TODO: use -p 50
test[1]="./code/tpcc -t 5 -m 10 -w 10 -s 0 -d 0 -o 10 -p 90 -r 0 -n"
test[2]="./code/tpcc -t 5 -m 10 -w 10 -s 0 -d 0 -o 90 -p 10 -r 0 -n"
#test[1]="./code/tpcc -t 5 -w 1 -s 0 -d 5 -o 90 -p 5 -r 0 -n"

# drop labyrith, it does not had much info

test_name[1]="TPCC_H"
test_name[2]="TPCC_L"

THREADS="1 2 4 8 12 13 14 16 22 24 26 27 28"

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
	timeout --signal=2 2m ${test[$1]} $2 >/dev/null
	# retry
	if [ $? -ne 0 ]; then
		pkill tpcc
		timeout --signal=2 3m ${test[$1]} $2  >/dev/null
	fi
	# retry
	if [ $? -ne 0 ]; then
		pkill tpcc
		timeout --signal=2 4m ${test[$1]} $2  >/dev/null
	fi
	pkill tpcc
}

function run_bench {
	rm -f parameters.txt stats_file stats_file.aux_thr
	for i in `seq $NB_BENCH`
	do
		for a in `seq 3 $SAMPLES`
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

# MAKEFILE_ARGS=" NDEBUG=1" \
#     $BUILD_SCRIPT stm-tinystm file >/dev/null
# run_bench STM
#
# MAKEFILE_ARGS="PERSISTENT_TM=1 NDEBUG=1" \
#     $BUILD_SCRIPT stm-tinystm file >/dev/null
# run_bench PSTM
#
# MAKEFILE_ARGS="SOLUTION=1 NDEBUG=1" \
# $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
# run_bench HTM
#
# MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=4 LOG_SIZE=$LOG_SIZE \
# NDEBUG=1" \
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
SORT_ALG=5 FILTER=0.50 NDEBUG=1" \
$BUILD_SCRIPT htm-sgl-nvm file >/dev/null
run_bench NVHTM_B

MAKEFILE_ARGS="SOLUTION=2 NDEBUG=1" \
$BUILD_SCRIPT htm-sgl-nvm file >/dev/null
run_bench PHTM

LOG_SIZE=1000000 # wrap around 10 times
MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE \
SORT_ALG=5 FILTER=0.50 NDEBUG=1" \
$BUILD_SCRIPT htm-sgl-nvm file >/dev/null
run_bench NVHTM_B_10

cd ./tests_nvm
./test_proc_STAMP.sh

rm $DATA_FOLDER/*CAT $DATA_FOLDER/*CAT.cols
