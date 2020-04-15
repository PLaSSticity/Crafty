#!/bin/bash

#############
# Compares the backward performance
# (processes data only)
#############

SAMPLES=5

# BANK parameters
NB_TRANSFERS=10000000
BANK_BUDGET=1000000
UPDATE_RATE=50
THREADS="1 2 4 8 10 14"
TX_SIZE=2

LOG_SIZE=100000

BUILD_SCRIPT=./build.sh
DATA_FOLDER=./dataBACKWARD_THREADS
SCRIPTS_FOLDER=../../scripts
PLOT_FOLDER=./tests/plot

NB_ACCOUNTS=65536

# dir where build.sh is
cd ..

for i in `seq $SAMPLES`
do
    for j in NVHTM0_01 NVHTM0_05 NVHTM0_1 NVHTM0_5 NVHTM_F NVHTM_W PHTM
    do
        $SCRIPTS_FOLDER/SCRIPT_concat_files.R \
            `ls $DATA_FOLDER/"$j"_*"$i"`
        mv cat.txt $DATA_FOLDER/"$j"_s_"$i"CAT
    done
done
$SCRIPTS_FOLDER/SCRIPT_put_column.R "\
            THREADS = a\$X.THREADS; \
			PA = a\$ABORTS / (a\$ABORTS + a\$COMMITS_HTM); \
			P_CON = a\$CONFLICT / a\$ABORTS * PA; \
			P_CAP = a\$CAPACITY / a\$ABORTS * PA; \
			P_OTH = PA - P_CON - P_CAP; \
			THROUGHPUT = a\$NB_TXS / a\$TIME; \
			PERC_BLOCKED = a\$PERC_BLOCKED; \
			a = cbind(THREADS, PA, P_CON, P_CAP, P_OTH, THROUGHPUT, \
                PERC_BLOCKED); " \
			`ls $DATA_FOLDER/*CAT`

for j in NVHTM0_01 NVHTM0_05 NVHTM0_1 NVHTM0_5 NVHTM_F NVHTM_W PHTM
do
    $SCRIPTS_FOLDER/SCRIPT_compute_AVG_ERR.R \
        `ls $DATA_FOLDER/"$j"*CAT.cols`
    mv avg.txt $DATA_FOLDER/test_"$j"_BACK.txt
done

gnuplot -c $PLOT_FOLDER/test_BACK_THREADS.gp      $DATA_FOLDER $NB_ACCOUNTS $UPDATE_RATE
gnuplot -c $PLOT_FOLDER/test_BACK_PREC_THREADS.gp $DATA_FOLDER $NB_ACCOUNTS $UPDATE_RATE

rm $DATA_FOLDER/*CAT $DATA_FOLDER/*CAT.cols
