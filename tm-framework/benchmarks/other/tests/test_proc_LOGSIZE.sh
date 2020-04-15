#!/bin/bash

#############
# Compares the backward performance
# (processes data only)
#############

SAMPLES=5

# BANK parameters
NB_TRANSFERS=100000000
BANK_BUDGET=1000000
UPDATE_RATE="1 10 50 90 99"
THREADS="14"
TX_SIZE="1"
READ_SIZE="64"

LOG_SIZE="10000 100000 1000000"

BUILD_SCRIPT=./build.sh
DATA_FOLDER=./dataLOGSIZE
SCRIPTS_FOLDER=../../scripts
PLOT_FOLDER=./tests/plot

NB_ACCOUNTS=2048

# dir where build.sh is
cd ..


for i in `seq $SAMPLES`
do
    for l in $LOG_SIZE
    do
        for j in NVHTM_B_"$l" NVHTM_B_50_"$l" NVHTM_F_"$l"
        do
            $SCRIPTS_FOLDER/SCRIPT_concat_files.R \
                `ls $DATA_FOLDER/"$j"_*"$i"`
            mv cat.txt $DATA_FOLDER/"$j"_s_"$i"CAT
        done
    done
done
$SCRIPTS_FOLDER/SCRIPT_put_column.R "\
			UPDATE_RATE = a\$UPDATE_RATE; \
			PA = a\$ABORTS / (a\$ABORTS + a\$COMMITS_HTM); \
			P_CON = a\$CONFLICT / a\$ABORTS * PA; \
			P_CAP = a\$CAPACITY / a\$ABORTS * PA; \
			P_EXP = a\$EXPLICIT / a\$ABORTS * PA; \
			P_OTH = PA - P_CON - P_CAP - P_EXP; \
			THROUGHPUT = a\$NB_TXS / a\$TIME; \
			PERC_BLOCKED = a\$PERC_BLOCKED; \
            NB_FLUSHES = a\$NB_FLUSHES + a\$X.NB_FLUSHES; \
            REMAIN = a\$REMAIN_LOG; \
			a = cbind(UPDATE_RATE, PA, P_CON, P_CAP, P_EXP, P_OTH, THROUGHPUT, \
                PERC_BLOCKED, NB_FLUSHES, REMAIN); " \
			`ls $DATA_FOLDER/*CAT`

for l in $LOG_SIZE
do
    for j in NVHTM_B_"$l" NVHTM_B_50_"$l" NVHTM_F_"$l"
    do
        $SCRIPTS_FOLDER/SCRIPT_compute_AVG_ERR.R \
            `ls $DATA_FOLDER/"$j"*CAT.cols`
        mv avg.txt $DATA_FOLDER/test_"$j".txt
    done
done

gnuplot -c $PLOT_FOLDER/test_LOGSIZE_X.gp       "LOGSIZE" $DATA_FOLDER $NB_ACCOUNTS $TX_SIZE $THREADS
gnuplot -c $PLOT_FOLDER/test_LOGSIZE_PA.gp      "LOGSIZE" $DATA_FOLDER $NB_ACCOUNTS $TX_SIZE $THREADS
gnuplot -c $PLOT_FOLDER/test_LOGSIZE_BLOCK.gp   "LOGSIZE" $DATA_FOLDER $NB_ACCOUNTS $TX_SIZE $THREADS
gnuplot -c $PLOT_FOLDER/test_LOGSIZE_FLUSHES.gp "LOGSIZE" $DATA_FOLDER $NB_ACCOUNTS $TX_SIZE $THREADS
gnuplot -c $PLOT_FOLDER/test_LOGSIZE_REM.gp     "LOGSIZE" $DATA_FOLDER $NB_ACCOUNTS $TX_SIZE $THREADS

rm $DATA_FOLDER/*CAT $DATA_FOLDER/*CAT.cols
