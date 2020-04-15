#!/bin/bash

#############
# Compares the maximum capacity of:
# HTM / PHTM / NV-HTM
#
#############

SAMPLES=10

# BANK parameters
NB_TRANSFERS=50000
BANK_BUDGET=1000000
UPDATE_RATE=100
THREADS="1" #"1 8 14 28"
TX_SIZE="1 2 4 8 12 16 24 32 40 48 64 128 256"

LOG_SIZE=40000000

BUILD_SCRIPT=./build.sh
DATA_FOLDER=./dataCAP
SCRIPTS_FOLDER=../../scripts
PLOT_FOLDER=./tests/plot

NB_ACCOUNTS=32768

# dir where build.sh is
cd ..

mkdir -p $DATA_FOLDER

### Call with 1-TX_SIZE 2-UPDATE_RATE 3-THREADS 4-NB_ACCOUNTS 5-NB_TRANSFERS
function run_bench {
	echo -ne "1\t0\t$4\t$2\t$1\n" >> parameters.txt
	ipcrm -M 0x00054321 >/dev/null
	READ_SIZE=$((2 * $1))
	# SIGINT
	timeout --signal=2 10m ./bank/bank -a $4 -d $5 -b $BANK_BUDGET \
		-u $2 -n $3 -s $1 -r $READ_SIZE -q -c >/dev/null
}

function bench {
	for i in `seq $SAMPLES`
	do
		for t in $THREADS
		do
			rm -f parameters.txt stats_file stats_file.aux_thr
			echo -ne "#SEQ\tNO_CONFL\tACCOUNTS\tUPDATE_RATE\tSIZE_TX\n" > parameters.txt
			for j in $TX_SIZE
			do
				run_bench $j $UPDATE_RATE $t $NB_ACCOUNTS $NB_TRANSFERS
			done
			mv parameters.txt     $DATA_FOLDER/"$1"_thr"$t"_par_s"$i"
			mv stats_file         $DATA_FOLDER/"$1"_thr"$t"_s"$i"
			mv stats_file.aux_thr $DATA_FOLDER/"$1"_thr"$t"_aux_s"$i"
		done
	done
}

# MAKEFILE_ARGS="SOLUTION=1 CACHE_ALIGN_POOL=1 NDEBUG=1" \
#     $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
# bench HTM

# NVHTM physical clocks, 2 threads sort,
MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=4 LOG_SIZE=$LOG_SIZE \
SORT_ALG=5 THRESHOLD=0.0 CACHE_ALIGN_POOL=1 NDEBUG=1" \
$BUILD_SCRIPT htm-sgl-nvm file >/dev/null
bench NVHTM
#
# MAKEFILE_ARGS="SOLUTION=2 CACHE_ALIGN_POOL=1 NDEBUG=1" \
# $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
# bench PHTM

for i in `seq $SAMPLES`
do
	for t in $THREADS
	do
		for j in HTM NVHTM PHTM
		do
			$SCRIPTS_FOLDER/SCRIPT_concat_files.R \
			`ls $DATA_FOLDER/"$j"_thr"$t"*s"$i"`
			mv cat.txt $DATA_FOLDER/"$j"_thr"$t"_s_"$i"CAT
		done
	done
done
$SCRIPTS_FOLDER/SCRIPT_put_column.R "\
	TX_SIZE = a\$SIZE_TX; \
	PA = a\$ABORTS / (a\$ABORTS + a\$COMMITS_HTM); \
	P_CON = a\$CONFLICT / a\$ABORTS * PA; \
	P_CAP = a\$CAPACITY / a\$ABORTS * PA; \
	P_OTH = PA - P_CON - P_CAP; \
	THROUGHPUT = a\$NB_TXS / a\$TIME; \
	a = cbind(TX_SIZE, PA, P_CON, P_CAP, P_OTH, THROUGHPUT); " \
	`ls $DATA_FOLDER/*CAT`

for t in $THREADS
do
	for j in HTM NVHTM PHTM
	do
		$SCRIPTS_FOLDER/SCRIPT_compute_AVG_ERR.R \
		`ls $DATA_FOLDER/"$j"_thr"$t"_*CAT.cols`
		echo `ls $DATA_FOLDER/"$j"_thr"$t"_*CAT.cols`
		mv avg.txt $DATA_FOLDER/test_"$j"_thr"$t"_CAP.txt
	done
done

for t in $THREADS
do
	gnuplot -c $PLOT_FOLDER/test_CAP.gp     $NB_ACCOUNTS $DATA_FOLDER $t
	gnuplot -c $PLOT_FOLDER/test_CAP_cap.gp $NB_ACCOUNTS $DATA_FOLDER $t
	gnuplot -c $PLOT_FOLDER/test_CAP_X.gp   $NB_ACCOUNTS $DATA_FOLDER $t
done

rm $DATA_FOLDER/*CAT $DATA_FOLDER/*CAT.cols
