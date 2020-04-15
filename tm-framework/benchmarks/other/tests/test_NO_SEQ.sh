#!/bin/bash

#############
# Compares the maximum capacity of:
# HTM / PHTM / NV-HTM
#
#############

SAMPLES=10

# BANK parameters
NB_TRANSFERS=10000
BANK_BUDGET=100000
UPDATE_RATE=100
THREADS="1 2 4 6"
TX_SIZE="2"
NAME="NOSEQ"

LOG_SIZE=100000

BUILD_SCRIPT=./build.sh
DATA_FOLDER=./dataNOSEQ
SCRIPTS_FOLDER=../../scripts
PLOT_FOLDER=./tests/plot

NB_ACCOUNTS=32768

# dir where build.sh is
cd ..

mkdir -p $DATA_FOLDER

### Call with 1-TX_SIZE 2-UPDATE_RATE 3-THREADS 4-NB_ACCOUNTS 5-NB_TRANSFERS
function run_bench {
	echo -ne "0\t0\t$4\t$2\t$1\n" >> parameters.txt
	ipcrm -M 0x00054321 >/dev/null
	timeout 3m ./bank/bank -a $4 -d $5 -b $BANK_BUDGET -u $2 -n $3 -s $1 >/dev/null
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

MAKEFILE_ARGS="SOLUTION=1" \
    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
bench HTM

# NVHTM physical clocks, 2 threads sort,
MAKEFILE_ARGS="SOLUTION=4 DO_CHECKPOINT=5 LOG_SIZE=$LOG_SIZE \
    SORT_ALG=4 THRESHOLD=0.0" \
    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
bench NVHTM

MAKEFILE_ARGS="SOLUTION=2" \
    $BUILD_SCRIPT htm-sgl-nvm file >/dev/null
bench PHTM

for i in `seq $SAMPLES`
do
    for j in HTM NVHTM PHTM
    do
        $SCRIPTS_FOLDER/SCRIPT_concat_files.R \
            `ls $DATA_FOLDER/"$j"_*"$i"`
        mv cat.txt $DATA_FOLDER/"$j"_s_"$i"CAT
    done
done
$SCRIPTS_FOLDER/SCRIPT_put_column.R "\
            THREADS = a\$.X.THREADS; \
			PA = a\$ABORTS / (a\$ABORTS + a\$COMMITS_HTM); \
			P_CON = a\$CONFLICT / a\$ABORTS * PA; \
			P_CAP = a\$CAPACITY / a\$ABORTS * PA; \
			P_OTH = PA - P_CON - P_CAP; \
			THROUGHPUT = a\$NB_TXS / a\$TIME; \
			a = cbind(THREADS, PA, P_CON, P_CAP, P_OTH, THROUGHPUT); " \
			`ls $DATA_FOLDER/*CAT`

for j in HTM NVHTM PHTM
do
    $SCRIPTS_FOLDER/SCRIPT_compute_AVG_ERR.R \
        `ls $DATA_FOLDER/"$j"*CAT.cols`
    mv avg.txt $DATA_FOLDER/test_"$j"_"$NAME".txt
done

# TODO:
gnuplot -c $PLOT_FOLDER/plot_thrs_X.gp  "$NAME" $DATA_FOLDER $NB_ACCOUNTS $TX_SIZE
gnuplot -c $PLOT_FOLDER/plot_thrs_PA.gp "$NAME" $DATA_FOLDER $NB_ACCOUNTS $TX_SIZE

rm $DATA_FOLDER/*CAT $DATA_FOLDER/*CAT.cols
